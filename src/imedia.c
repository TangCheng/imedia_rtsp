#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include <mpi_vda.h>
#include <messages.h>
#include "imedia.h"
#if defined(HI3518) || defined(HI3516)
#include "hi3518/media_sys_ctrl.h"
#include "hi3518/media_video.h"
#include "hi3518/media_osd.h"
#endif
#include "rtsp/rtsp.h"
#include "video_param_change_handler.h"
#include "media-ircut.h"

#define OSD_BUFFER_LENGTH 4096

typedef struct _IpcamIMediaPrivate
{
    IpcamMediaSysCtrl *sys_ctrl;
    IpcamMediaVideo *video;
    IpcamOSD *osd;
	gboolean initialized;
    gchar *osd_buffer;
    IpcamRtsp *rtsp;
    time_t last_time;
    regex_t reg;
    gchar *model;
    gchar *train_num;
    gchar *carriage_num;
    gchar *position_num;
    StreamDescriptor stream_desc[STREAM_CHN_LAST];
    OD_REGION_INFO od_rgn_info[VDA_OD_RGN_NUM_MAX];
	MediaIrCut *ircut;
} IpcamIMediaPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamIMedia, ipcam_imedia, IPCAM_BASE_APP_TYPE)

static void ipcam_imedia_before(IpcamBaseService *base_service);
static void ipcam_imedia_in_loop(IpcamBaseService *base_service);
static void message_handler(GObject *obj, IpcamMessage* msg, gboolean timeout);
static void ipcam_imedia_query_day_night_mode(IpcamIMedia *imedia);
static void ipcam_imedia_query_osd_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_query_szyc_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_query_baseinfo_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_request_users(IpcamIMedia *imedia);
static void ipcam_imedia_request_rtsp_port(IpcamIMedia *imedia);
static void ipcam_imedia_query_rtsp_auth(IpcamIMedia *imedia);
static void ipcam_imedia_query_od_param(IpcamIMedia *imedia);
static void ipcam_imedia_query_video_param(IpcamIMedia *imedia);
static void ipcam_imedia_set_users(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_set_rtsp_port(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_set_rtsp_auth(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_osd_display_video_data(GObject *obj);

static void ipcam_imedia_finalize(GObject *object)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(object));

    g_clear_object(&priv->rtsp);
    g_clear_object(&priv->sys_ctrl);
    g_clear_object(&priv->video);

	ipcam_osd_destroy(priv->osd);
    g_free(priv->model);
    g_free(priv->train_num);
    g_free(priv->carriage_num);
    g_free(priv->position_num);
    regfree(&priv->reg);
    gint chn = MASTER_CHN;
    for (chn = MASTER_CHN; chn < STREAM_CHN_LAST; chn++)
    {
        g_free((gpointer)priv->stream_desc[chn].v_desc.path);
        g_free((gpointer)priv->stream_desc[chn].v_desc.resolution);
    }
    g_free(priv->osd_buffer);
	if (priv->ircut)
		media_ircut_free(priv->ircut);
    G_OBJECT_CLASS(ipcam_imedia_parent_class)->finalize(object);
}

static void ipcam_imedia_init(IpcamIMedia *self)
{
	IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(self);
    const gchar *pattern = "InsBr.*InsFr[^\n]*[\n]+([^\n]*)[\n]([^\n]*)[\n]";
    int i;

	priv->initialized = FALSE;

    regcomp(&priv->reg, pattern, REG_EXTENDED | REG_NEWLINE);
    priv->sys_ctrl = g_object_new(IPCAM_MEDIA_SYS_CTRL_TYPE, NULL);
    priv->video = g_object_new(IPCAM_MEDIA_VIDEO_TYPE, "app", self, NULL);

	priv->model = NULL;
    priv->train_num = NULL;
    priv->carriage_num = NULL;
    priv->position_num = NULL;
    priv->stream_desc[MASTER].type = VIDEO_STREAM;
    priv->stream_desc[MASTER].v_desc.format = VIDEO_FORMAT_H264;
    priv->stream_desc[MASTER].v_desc.img_attr.brightness = -1;
    priv->stream_desc[MASTER].v_desc.img_attr.chrominance = -1;
    priv->stream_desc[MASTER].v_desc.img_attr.contrast = -1;
    priv->stream_desc[MASTER].v_desc.img_attr.saturation = -1;
    priv->stream_desc[MASTER].v_desc.img_attr.b3ddnr = -1;
    priv->stream_desc[MASTER].v_desc.img_attr.watermark = -1;
    
    priv->stream_desc[SLAVE].type = VIDEO_STREAM;
    priv->stream_desc[SLAVE].v_desc.format = VIDEO_FORMAT_H264;
    time(&priv->last_time);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_base_info", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_misc", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "add_users", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_users", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "del_users", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_event_cover", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_video", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_osd", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_image", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_szyc", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_day_night_mode", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);

    for (i = 0; i < VDA_OD_RGN_NUM_MAX; i++) {
        priv->od_rgn_info[i].enable = FALSE;
        priv->od_rgn_info[i].sensitivity = 50;
        priv->od_rgn_info[i].rect.left = 0;
        priv->od_rgn_info[i].rect.top = 0;
        priv->od_rgn_info[i].rect.width = 100;
        priv->od_rgn_info[i].rect.height = 100;
    }
    priv->osd_buffer = g_malloc(OSD_BUFFER_LENGTH);
	priv->ircut = media_ircut_new(256, 15);
	g_print("IrCut hardware %s\n", priv->ircut ? "found" : "not found");
}
static void ipcam_imedia_class_init(IpcamIMediaClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_imedia_finalize;

    IpcamBaseServiceClass *base_service_class = IPCAM_BASE_SERVICE_CLASS(klass);
    base_service_class->before = &ipcam_imedia_before;
    base_service_class->in_loop = &ipcam_imedia_in_loop;
}

static void ipcam_imedia_before(IpcamBaseService *base_service)
{
    IpcamIMedia *imedia = IPCAM_IMEDIA(base_service);
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    ipcam_media_sys_ctrl_init_media_system(priv->sys_ctrl);

    ipcam_imedia_query_baseinfo_parameter(imedia);
    ipcam_imedia_request_users(imedia);
    ipcam_imedia_query_rtsp_auth(imedia);
    ipcam_imedia_request_rtsp_port(imedia);
    ipcam_imedia_query_od_param(imedia);
    ipcam_imedia_query_video_param(imedia);
    priv->rtsp = g_object_new(IPCAM_TYPE_RTSP, NULL);
    ipcam_imedia_query_szyc_parameter(imedia);
    ipcam_imedia_query_osd_parameter(imedia);
    ipcam_imedia_query_baseinfo_parameter(imedia);
	ipcam_imedia_query_day_night_mode(imedia);
}

static void ipcam_imedia_in_loop(IpcamBaseService *base_service)
{
    IpcamIMedia *imedia = IPCAM_IMEDIA(base_service);
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    time_t now;
    int i;
	gchar osdBuf[128] = {0};
    gchar timeBuf[64] = {0};

	if (!priv->initialized)
		return;

	if (priv->ircut) {
		if (media_ircut_poll(priv->ircut)) {
			gboolean ir_status = media_ircut_get_status(priv->ircut);

			ipcam_media_video_set_color2grey(priv->video,
											 ir_status);
		}
	}

    time(&now);
    if (priv->last_time != now)
    {
        priv->last_time = now;
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

        if (!priv->model) {
			g_strlcat(osdBuf, timeBuf, sizeof(osdBuf));
		}
		else {
			if (g_ascii_strncasecmp(priv->model, "DCTX", 4) == 0) {
				g_strlcpy(osdBuf, timeBuf, sizeof(timeBuf));
				if (priv->train_num) {
					g_strlcat(osdBuf, " ", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->train_num, sizeof(osdBuf));
				}
				if (priv->carriage_num) {
					g_strlcat(osdBuf, " ", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->carriage_num, sizeof(osdBuf));
				}
				if (priv->position_num) {
					g_strlcat(osdBuf, "-", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->position_num, sizeof(osdBuf));
				}
				//g_strlcat(osdBuf, " ", sizeof(osdBuf));
			}
			else if (g_ascii_strncasecmp(priv->model, "DTTX", 4) == 0) {
				if (priv->train_num) {
					//g_strlcpy(osdBuf, "", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->train_num, sizeof(osdBuf));
				}
				if (priv->position_num) {
					g_strlcat(osdBuf, " ", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->position_num, sizeof(osdBuf));
				}
				g_strlcat(osdBuf, " ", sizeof(osdBuf));
				g_strlcat(osdBuf, timeBuf, sizeof(osdBuf));
			}
            else {
                g_strlcat(osdBuf, timeBuf, sizeof(osdBuf));
            }
		}

		for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
		{
			ipcam_osd_set_item_text(priv->osd, i, "datetime", osdBuf);
		}
	}
}

static void message_handler(GObject *obj, IpcamMessage* msg, gboolean timeout)
{
    g_return_if_fail(IPCAM_IS_IMEDIA(obj));

    if (!timeout && msg)
    {
        IpcamResponseMessage *res_msg = IPCAM_RESPONSE_MESSAGE(msg);
        gchar *action = NULL;
        g_object_get(res_msg, "action", &action, NULL);
        g_return_if_fail((NULL != action));
        JsonNode *body = NULL;
        g_object_get(res_msg, "body", &body, NULL);
        g_return_if_fail((NULL != body));

        if (g_str_equal(action, "get_szyc"))
        {
            ipcam_imedia_got_szyc_parameter(IPCAM_IMEDIA(obj), body);
        }
        else if (g_str_equal(action, "get_osd"))
        {
            ipcam_imedia_got_osd_parameter(IPCAM_IMEDIA(obj), body);
        }
        else if (g_str_equal(action, "get_base_info"))
        {
            ipcam_imedia_got_baseinfo_parameter(IPCAM_IMEDIA(obj), body);
        }
        else if (g_str_equal("get_network", action))
        {
            ipcam_imedia_set_rtsp_port(IPCAM_IMEDIA(obj), body);
        }
        else if (g_str_equal("get_users", action))
        {
            ipcam_imedia_set_users(IPCAM_IMEDIA(obj), body);
        }
        else if (g_str_equal("get_misc", action))
        {
            ipcam_imedia_set_rtsp_auth(IPCAM_IMEDIA(obj), body);
        }
        else if (g_str_equal("get_event_cover", action))
        {
            ipcam_imedia_got_od_param(IPCAM_IMEDIA(obj), body, FALSE);
        }
        else if (g_str_equal("get_video", action))
        {
            ipcam_imedia_got_video_param(IPCAM_IMEDIA(obj), body, FALSE);
        }
		else if (g_str_equal("get_day_night_mode", action))
		{
			ipcam_imedia_got_day_night_mode_parameter(IPCAM_IMEDIA(obj), body);
		}
        else
        {
            g_warning("Unhandled message: %s\n", action);
        }
	}
}

static void ipcam_imedia_query_param(IpcamIMedia *imedia, IpcamRequestMessage *rq_msg, JsonBuilder *builder)
{
    gchar *token = (gchar *)ipcam_base_app_get_config(IPCAM_BASE_APP(imedia), "token");
    JsonNode *body = json_builder_get_root(builder);
    g_object_set(G_OBJECT(rq_msg), "body", body, NULL);
    ipcam_base_app_send_message(IPCAM_BASE_APP(imedia), IPCAM_MESSAGE(rq_msg), "iconfig", token,
                                message_handler, 20);
}

static void ipcam_imedia_query_szyc_parameter(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                               "action", "get_szyc", NULL);
    
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "train_num");
    json_builder_add_string_value(builder, "carriage_num");
    json_builder_add_string_value(builder, "position_num");
    json_builder_end_array(builder);
    json_builder_end_object(builder);
    
    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_query_day_night_mode(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                               "action", "get_day_night_mode", NULL);
    
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "night_mode_threshold");
    json_builder_add_string_value(builder, "ir_intensity");
    json_builder_end_array(builder);
    json_builder_end_object(builder);
    
    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_query_osd_parameter(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                               "action", "get_osd", NULL);
    
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "master:datetime");
    json_builder_add_string_value(builder, "master:device_name");
    json_builder_add_string_value(builder, "master:comment");
    json_builder_add_string_value(builder, "master:frame_rate");
    json_builder_add_string_value(builder, "master:bit_rate");
    json_builder_add_string_value(builder, "slave:datetime");
    json_builder_add_string_value(builder, "slave:device_name");
    json_builder_add_string_value(builder, "slave:comment");
    json_builder_add_string_value(builder, "slave:frame_rate");
    json_builder_add_string_value(builder, "slave:bit_rate");
    json_builder_end_array(builder);
    json_builder_end_object(builder);
    
    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_query_baseinfo_parameter(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                               "action", "get_base_info", NULL);
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "device_name");
    json_builder_add_string_value(builder, "comment");
    json_builder_add_string_value(builder, "model");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_request_users(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                              "action", "get_users", NULL);
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "password");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_request_rtsp_port(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                              "action", "get_network", NULL);
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "port");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_query_rtsp_auth(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                              "action", "get_misc", NULL);
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "rtsp_auth");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_query_od_param(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                              "action", "get_event_cover", NULL);
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "region1");
    json_builder_add_string_value(builder, "region2");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

static void ipcam_imedia_query_video_param(IpcamIMedia *imedia)
{
    IpcamRequestMessage *rq_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
                                              "action", "get_video", NULL);
    JsonBuilder *builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "profile");
    json_builder_add_string_value(builder, "flip");
    json_builder_add_string_value(builder, "mirror");
    json_builder_add_string_value(builder, "main_profile");
    json_builder_add_string_value(builder, "sub_profile");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ipcam_imedia_query_param(imedia, rq_msg, builder);
    g_object_unref(rq_msg);
    g_object_unref(builder);
}

void ipcam_imedia_got_osd_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *profile_object;
    JsonObject *osd_object;
    JsonObject *res_object;
    JsonObject *color_object;
    GList *members, *item;
    const char *key[] = {"master", "slave"};
    gint i = 0;

    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    for (i = 0; i < ARRAY_SIZE(key); i++)
    {
        if (json_object_has_member(res_object, key[i]))
        {
			IpcamOSDStream *stream = ipcam_osd_lookup_stream(priv->osd, i);

			profile_object = json_object_get_object_member(res_object, key[i]);
			members = json_object_get_members(profile_object);            
            for (item = g_list_first(members); item; item = g_list_next(item))
            {
                const gchar *name = item->data;
				IpcamOSDItem *item;
				gboolean enabled;
				int font_size;
				int left;
				int top;

				item = ipcam_osd_stream_add_item(stream, name);

                osd_object = json_object_get_object_member(profile_object, name);
                enabled = json_object_get_boolean_member(osd_object, "isshow");
                font_size = json_object_get_int_member(osd_object, "size");
                left = json_object_get_int_member(osd_object, "left");
                top = json_object_get_int_member(osd_object, "top");

				ipcam_osd_item_set_font_size(item, font_size);
				ipcam_osd_item_set_position(item, left, top);

				color_object = json_object_get_object_member(osd_object, "color");
                if (color_object)
                {
					SDL_Color color;

					color.r = json_object_get_int_member(color_object, "red");
                    color.g = json_object_get_int_member(color_object, "green");
                    color.b = json_object_get_int_member(color_object, "blue");
                    color.a = json_object_get_int_member(color_object, "alpha");

					ipcam_osd_item_set_fgcolor(item, &color);
                }

				ipcam_osd_item_disable(item);
				if (enabled) {
					ipcam_osd_item_enable(item);
					ipcam_osd_item_draw_text(item);
				}
            }
            g_list_free(members);
        }
    }
}

void ipcam_imedia_got_day_night_mode_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *items_obj;
	guint threshold = 0;
	guint ir_intensity = 0;

    items_obj = json_object_get_object_member(json_node_get_object(body), "items");

    if (json_object_has_member(items_obj, "night_mode_threshold"))
    {
        threshold = json_object_get_int_member(items_obj, "night_mode_threshold");

		if (priv->ircut)
			media_ircut_set_sensitivity(priv->ircut, threshold);
    }
    if (json_object_has_member(items_obj, "ir_intensity"))
    {
        ir_intensity = json_object_get_int_member(items_obj, "ir_intensity");

		if (priv->ircut)
			media_ircut_set_ir_intensity(priv->ircut, ir_intensity);
	}
}

void ipcam_imedia_got_szyc_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *items_obj;
    const gchar *train_num = NULL;
    const gchar *carriage_num = NULL;
    const gchar *position_num = NULL;
    int i;

    items_obj = json_object_get_object_member(json_node_get_object(body), "items");

    if (json_object_has_member(items_obj, "train_num"))
    {
        train_num = json_object_get_string_member(items_obj, "train_num");

		g_free(priv->train_num);
		priv->train_num = g_strdup(train_num);
    }
    if (json_object_has_member(items_obj, "carriage_num"))
    {
        carriage_num = json_object_get_string_member(items_obj, "carriage_num");

		g_free(priv->carriage_num);
		priv->carriage_num = g_strdup(carriage_num);
	}
    if (json_object_has_member(items_obj, "position_num"))
    {
        position_num = json_object_get_string_member(items_obj, "position_num");

		g_free(priv->position_num);
		priv->position_num = g_strdup(position_num);
	}
}

void ipcam_imedia_got_image_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *res_object;
    IpcamMediaImageAttr *imgattr = &priv->stream_desc[MASTER].v_desc.img_attr;
	const gchar *range_str = NULL;
	guint range = 0;

	range_str = ipcam_base_app_get_config(IPCAM_BASE_APP(imedia), "imedia_rtsp:image_setting_range");
	if (range_str)
		range = strtoul(range_str, NULL, 0);

    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    if (json_object_has_member(res_object, "brightness")) {
        guint32 val = (guint32)json_object_get_int_member(res_object, "brightness");
		imgattr->brightness = range ? (val * 100 / range) : val;
    }
    if (json_object_has_member(res_object, "chrominance")) {
        guint32 val = (gint32)json_object_get_int_member(res_object, "chrominance");
		imgattr->chrominance = range ? (val * 100 / range) : val;
    }
    if (json_object_has_member(res_object, "contrast")) {
        guint32 val = (gint32)json_object_get_int_member(res_object, "contrast");
		imgattr->contrast = range ? (val * 100 / range) : val;
    }
    if (json_object_has_member(res_object, "saturation")) {
        guint32 val = (gint32)json_object_get_int_member(res_object, "saturation");
		imgattr->saturation = range ? (val * 100 / range) : val;
    }
    if (json_object_has_member(res_object, "3ddnr")) {
        imgattr->b3ddnr = json_object_get_boolean_member(res_object, "3ddnr");
        ipcam_media_video_param_change(priv->video, priv->stream_desc, priv->od_rgn_info);
    }
    if (json_object_has_member(res_object, "watermark")) {
        imgattr->watermark = json_object_get_boolean_member(res_object, "watermark");
    }

    ipcam_media_video_set_image_parameter(priv->video, imgattr);
}

void ipcam_imedia_got_baseinfo_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *res_object;
	int i;
    
    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    const gchar *device_name = NULL;
    if (json_object_has_member(res_object, "device_name"))
    {
        device_name = json_object_get_string_member(res_object, "device_name");
    }

	for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++) {
		if (NULL != device_name && strlen(device_name) > 0) {
			ipcam_osd_set_item_text(priv->osd, i, "device_name", device_name);
        }
		else {
			ipcam_osd_set_item_text(priv->osd, i, "device_name", "");
		}
    }

    const gchar *comment = NULL;
    comment = json_object_get_string_member(res_object, "comment");

	for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++) {
		if (NULL != comment && strlen(comment) > 0) {
			ipcam_osd_set_item_text(priv->osd, i, "comment", comment);
        }
		else {
			ipcam_osd_set_item_text(priv->osd, i, "comment", "");
		}
    }

    const gchar *model = NULL;
    model = json_object_get_string_member(res_object, "model");
    if (NULL != model && strlen(model) > 0)
    {
        g_free(priv->model);
        priv->model = g_strdup(model);
    }
}

void ipcam_imedia_got_misc_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    JsonObject *res_object;

    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    if (json_object_has_member(res_object, "rtsp_auth")) {
        ipcam_imedia_set_rtsp_auth(imedia, body);
    }
}

void ipcam_imedia_got_set_users_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonArray *item_arr;
    JsonObject *obj;
    int i;

    item_arr = json_object_get_array_member(json_node_get_object(body), "items");

    for (i = 0; i < json_array_get_length (item_arr); i++) {
        const char *username = NULL;
        const char *password = NULL;

        obj = json_array_get_object_element (item_arr, i);
        if (obj && json_object_has_member(obj, "username")) {
            username = json_object_get_string_member(obj, "username");
            if (json_object_has_member(obj, "username"))
            {
                password = json_object_get_string_member(obj, "password");
            }
        }

        ipcam_rtsp_insert_user(priv->rtsp, username, password);
    }
}

void ipcam_imedia_got_del_users_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonArray *item_arr;
    JsonObject *obj;
    int i;

    item_arr = json_object_get_array_member(json_node_get_object(body), "items");

    for (i = 0; i < json_array_get_length (item_arr); i++) {
        const char *username;

        obj = json_array_get_object_element (item_arr, i);
        username = json_object_get_string_member(obj, "username");

        ipcam_rtsp_delete_user(priv->rtsp, username);
    }
}

static void ipcam_imedia_set_rtsp_port(IpcamIMedia *imedia, JsonNode *body)
{
    JsonObject *items;
    JsonObject *server_port;
    guint rtsp_port;

    items = json_object_get_object_member(json_node_get_object(body), "items");

    g_return_if_fail(items);

    server_port = json_object_get_object_member(items, "port");

    g_return_if_fail(server_port && json_object_has_member(server_port, "rtsp"));

    rtsp_port = json_object_get_int_member(server_port, "rtsp");

    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    ipcam_rtsp_set_port(priv->rtsp, rtsp_port);
}

static void proc_each_user(JsonArray *array, guint index_, JsonNode *element_node, gpointer user_data)
{
    const gchar *username = NULL;
    const gchar *password = NULL;
    JsonObject *obj = json_node_get_object(element_node);
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(user_data));

    if (json_object_has_member(obj, "username")) {
        username = json_object_get_string_member(obj, "username");
        if (json_object_has_member(obj, "password"))
            password = json_object_get_string_member(obj, "password");

        ipcam_rtsp_insert_user(priv->rtsp, username, password);
    }
}

static void ipcam_imedia_set_users(IpcamIMedia *irtsp, JsonNode *body)
{
    JsonArray *users = json_object_get_array_member(json_node_get_object(body), "items");
    json_array_foreach_element(users, proc_each_user, irtsp);
}

static void ipcam_imedia_set_rtsp_auth(IpcamIMedia *imedia, JsonNode *body)
{
    JsonObject *res_obj = json_object_get_object_member(json_node_get_object(body), "items");
    gboolean rtsp_auth;

    if (json_object_has_member(res_obj, "rtsp_auth")) {
        rtsp_auth = json_object_get_boolean_member(res_obj, "rtsp_auth");

        IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
        ipcam_rtsp_set_auth(priv->rtsp, rtsp_auth);
    }
}

static void ipcam_imedia_parse_profile(IpcamIMedia *imedia, const gchar *profile)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    if (g_str_equal(profile, "baseline"))
    {
        priv->stream_desc[MASTER].v_desc.profile =
            priv->stream_desc[SLAVE].v_desc.profile = BASELINE_PROFILE;
    }
    else if (g_str_equal(profile, "main"))
    {
        priv->stream_desc[MASTER].v_desc.profile =
            priv->stream_desc[SLAVE].v_desc.profile = MAIN_PROFILE;
    }
    else if (g_str_equal(profile, "high"))
    {
        priv->stream_desc[MASTER].v_desc.profile =
            priv->stream_desc[SLAVE].v_desc.profile = HIGH_PROFILE;
    }
    else
    {
        priv->stream_desc[MASTER].v_desc.profile =
            priv->stream_desc[SLAVE].v_desc.profile = BASELINE_PROFILE;
        g_warn_if_reached();
    }
}

static void ipcam_imedia_parse_resolution(IpcamIMedia *imedia, const gchar *resolution_name, enum StreamChannel chn)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    if (g_str_equal(resolution_name, "UXGA"))
    {
        priv->stream_desc[chn].v_desc.image_width = 1600;
        priv->stream_desc[chn].v_desc.image_height = 1200;
    }
    else if (g_str_equal(resolution_name, "1080P"))
    {
        priv->stream_desc[chn].v_desc.image_width = 1920;
        priv->stream_desc[chn].v_desc.image_height = 1080;
    }
    else if (g_str_equal(resolution_name, "960H"))
    {
        priv->stream_desc[chn].v_desc.image_width = 1280;
        priv->stream_desc[chn].v_desc.image_height = 960;
    }
    else if (g_str_equal(resolution_name, "720P"))
    {
        priv->stream_desc[chn].v_desc.image_width = 1280;
        priv->stream_desc[chn].v_desc.image_height = 720;
    }
    else if (g_str_equal(resolution_name, "D1"))
    {
        priv->stream_desc[chn].v_desc.image_width = 704;
        priv->stream_desc[chn].v_desc.image_height = 576;
    }
    else if (g_str_equal(resolution_name, "CIF"))
    {
        priv->stream_desc[chn].v_desc.image_width = 352;
        priv->stream_desc[chn].v_desc.image_height = 288;
    }
    else
    {
        g_warn_if_reached();
    }
}

static void ipcam_imedia_parse_bit_rate(IpcamIMedia *imedia, const gchar *bit_rate, enum StreamChannel chn)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    if (g_str_equal(bit_rate, "CBR"))
    {
        priv->stream_desc[chn].v_desc.bit_rate_type = CONSTANT_BIT_RATE;
    }
    else if (g_str_equal(bit_rate, "VBR"))
    {
        priv->stream_desc[chn].v_desc.bit_rate_type = VARIABLE_BIT_RATE;
    }
    else
    {
        g_warn_if_reached();
    }
}

static void ipcam_imedia_parse_stream(IpcamIMedia *imedia, JsonObject *obj, enum StreamChannel chn)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    const gchar *resolution;
    const gchar *bit_rate_ctrl;

    if (json_object_has_member(obj, "resolution")) {
        resolution = json_object_get_string_member(obj, "resolution");
        ipcam_imedia_parse_resolution(imedia, resolution, chn);
		g_free((gpointer)priv->stream_desc[chn].v_desc.resolution);
        priv->stream_desc[chn].v_desc.resolution = g_strdup(resolution);
    }
    if (json_object_has_member(obj, "bit_rate")) {
        bit_rate_ctrl = json_object_get_string_member(obj, "bit_rate");
        ipcam_imedia_parse_bit_rate(imedia, bit_rate_ctrl, chn);
    }
    if (json_object_has_member(obj, "frame_rate")) {
        priv->stream_desc[chn].v_desc.frame_rate = json_object_get_int_member(obj, "frame_rate");
    }
    if (json_object_has_member(obj, "bit_rate_value")) {
        priv->stream_desc[chn].v_desc.bit_rate = json_object_get_int_member(obj, "bit_rate_value");
    }
    if (json_object_has_member(obj, "stream_path")) {
		g_free((gpointer)priv->stream_desc[chn].v_desc.path);
        priv->stream_desc[chn].v_desc.path = g_strdup(json_object_get_string_member(obj, "stream_path"));
    }
}

void ipcam_imedia_got_od_param(IpcamIMedia *imedia, JsonNode *body, gboolean is_notice)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *res_object;
    int i;

    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    for (i = 0; i < VDA_OD_RGN_NUM_MAX; i++) {
        priv->od_rgn_info[i].enable = FALSE;
    }

    for (i = 0; i < VDA_OD_RGN_NUM_MAX; i++) {
        char rgn_name[16];
        sprintf(rgn_name, "region%d", i + 1);
        if (json_object_has_member(res_object, rgn_name)) {
            JsonObject *rgn_obj = json_object_get_object_member(res_object, rgn_name);

            if (json_object_has_member(rgn_obj, "enable"))
                priv->od_rgn_info[i].enable = json_object_get_boolean_member(rgn_obj, "enable");
            if (json_object_has_member(rgn_obj, "sensitivity"))
                priv->od_rgn_info[i].sensitivity = json_object_get_int_member(rgn_obj, "sensitivity");
            if (json_object_has_member(rgn_obj, "rect")) {
                JsonObject *rect_obj = json_object_get_object_member(rgn_obj, "rect");

                if (json_object_has_member(rect_obj, "left"))
                    priv->od_rgn_info[i].rect.left = json_object_get_int_member(rect_obj, "left");
                if (json_object_has_member(rect_obj, "top"))
                    priv->od_rgn_info[i].rect.top = json_object_get_int_member(rect_obj, "top");
                if (json_object_has_member(rect_obj, "width"))
                    priv->od_rgn_info[i].rect.width = json_object_get_int_member(rect_obj, "width");
                if (json_object_has_member(rect_obj, "height"))
                    priv->od_rgn_info[i].rect.height = json_object_get_int_member(rect_obj, "height");
            }
        }
    }

    if (is_notice) {
        ipcam_media_video_param_change(priv->video, priv->stream_desc, priv->od_rgn_info);
    }
}

#define DEFAULT_FONT    "/usr/share/fonts/truetype/droid/DroidSansFallback.ttf"
#define SIMSUN_FONT     "/usr/share/fonts/truetype/simsun.ttf"

void ipcam_imedia_got_video_param(IpcamIMedia *imedia, JsonNode *body, gboolean is_notice)
{
    JsonObject *res_obj = json_object_get_object_member(json_node_get_object(body), "items");
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
	const gchar *font = DEFAULT_FONT;

    if (priv->model) {
        if (g_ascii_strncasecmp(priv->model, "DTTX", 4) == 0) {
            font = SIMSUN_FONT;
        }
    }

    if (json_object_has_member(res_obj, "profile"))
    {
        const gchar *profile = json_object_get_string_member(res_obj, "profile");
        ipcam_imedia_parse_profile(imedia, profile);
    }
    if (json_object_has_member(res_obj, "flip"))
    {
        gboolean bValue = json_object_get_boolean_member(res_obj, "flip");
        priv->stream_desc[MASTER].v_desc.flip = bValue;
        priv->stream_desc[SLAVE].v_desc.flip = bValue;
    }
    if (json_object_has_member(res_obj, "mirror"))
    {
        gboolean bValue = json_object_get_boolean_member(res_obj, "mirror");
        priv->stream_desc[MASTER].v_desc.mirror = bValue;
        priv->stream_desc[SLAVE].v_desc.mirror = bValue;
    }
    JsonObject *stream_obj = NULL;
    if (json_object_has_member(res_obj, "main_profile"))
    {
        stream_obj = json_object_get_object_member(res_obj, "main_profile");
        ipcam_imedia_parse_stream(imedia, stream_obj, MASTER);
    }
    if (json_object_has_member(res_obj, "sub_profile"))
    {
        stream_obj = json_object_get_object_member(res_obj, "sub_profile");
        ipcam_imedia_parse_stream(imedia, stream_obj, SLAVE);
    }
    if (is_notice == FALSE)
    {
        int i;

        ipcam_media_video_start_livestream(priv->video, priv->stream_desc, priv->od_rgn_info);
		priv->osd = ipcam_osd_new(font);
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
        {
			IpcamOSDStream *stream;

			stream = ipcam_osd_add_stream(priv->osd, i);
			ipcam_osd_stream_set_image_size(stream,
											priv->stream_desc[i].v_desc.image_width,
											priv->stream_desc[i].v_desc.image_height);
        }
        ipcam_base_app_add_timer(IPCAM_BASE_APP(imedia), "osd_display_video_data", "1",
                                 ipcam_imedia_osd_display_video_data);
        ipcam_rtsp_set_user_data(priv->rtsp, imedia);
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
        {
            ipcam_rtsp_set_stream_desc(priv->rtsp, i, &priv->stream_desc[i]);
        }
        ipcam_rtsp_start_server(priv->rtsp);

		priv->initialized = TRUE;
    }
    else
    {
        int i;

        for (i = 0; i < STREAM_CHN_LAST; i++) {
			IpcamOSDStream *stream = ipcam_osd_lookup_stream(priv->osd, i);
			if (stream) {
				ipcam_osd_stream_set_image_size(stream,
												priv->stream_desc[i].v_desc.image_width,
												priv->stream_desc[i].v_desc.image_height);
			}
		}
        ipcam_media_video_param_change(priv->video, priv->stream_desc, priv->od_rgn_info);
    }
}

static void ipcam_imedia_osd_lookup_video_run_info(IpcamIMedia *imedia,
                                                   gchar *buffer)
{
    regmatch_t pmatch[4];
    const size_t nmatch = sizeof(pmatch) / sizeof(pmatch[0]);
    gint status;
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);

    status = regexec(&priv->reg, buffer, nmatch, pmatch, 0);
    if (REG_NOMATCH != status)
    {
        gchar *p;
        gint id;
        gint bit_rate_val;
        gint frame_rate_val;
        int i;

        for (i = 1; i < nmatch; i++) {
            if (pmatch[i].rm_so == -1 || pmatch[i].rm_eo == -1)
                break;
            p = &buffer[pmatch[i].rm_so];
            if (sscanf(p, "%d%d%d", &id, &bit_rate_val, &frame_rate_val) == 3) {
				char br_buf[32];
				char fr_buf[32];

				snprintf(br_buf, sizeof(br_buf), "BR=%dK", bit_rate_val);
				snprintf(fr_buf, sizeof(fr_buf), "FR=%d", frame_rate_val);
				ipcam_osd_set_item_text(priv->osd, id, "frame_rate", fr_buf);
				ipcam_osd_set_item_text(priv->osd, id, "bit_rate", br_buf);
            }
        }
    }
}
static void ipcam_imedia_osd_display_video_data(GObject *obj)
{
    g_return_if_fail(IPCAM_IS_IMEDIA(obj));
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(obj));

    if (NULL != priv->osd_buffer)
    {
        FILE *rc_file = fopen("/proc/umap/rc", "r");
        if (NULL != rc_file)
        {
            if (fread(priv->osd_buffer, 1, OSD_BUFFER_LENGTH, rc_file) > 0)
            {
                ipcam_imedia_osd_lookup_video_run_info(IPCAM_IMEDIA(obj), priv->osd_buffer);
            }        
            fclose(rc_file);
        }
    }
}

StreamDescriptor *ipcam_imedia_get_stream_info(IpcamIMedia *imedia, StreamChannel chn)
{
    g_return_val_if_fail(IPCAM_IS_IMEDIA(imedia), NULL);
    g_return_val_if_fail(chn >= 0 && chn < STREAM_CHN_LAST, NULL);

    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);

    return &priv->stream_desc[chn];
}
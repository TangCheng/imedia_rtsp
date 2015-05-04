#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include <messages.h>
#include "imedia.h"
#include "interface/media_sys_ctrl_interface.h"
#include "interface/media_video_interface.h"
#include "interface/media_osd_interface.h"
#include "stream_descriptor.h"
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
    IpcamIMediaSysCtrl *sys_ctrl;
    IpcamIVideo *video;
    IpcamIOSD *osd[STREAM_CHN_LAST];
    gchar *osd_buffer;
    IpcamRtsp *rtsp;
    time_t last_time;
    regex_t reg;
    gchar *bit_rate[STREAM_CHN_LAST];
    gchar *frame_rate[STREAM_CHN_LAST];
    gchar *train_num;
    gchar *carriage_num;
    gchar *position_num;
    StreamDescriptor stream_desc[STREAM_CHN_LAST];
	MediaIrCut *ircut;
} IpcamIMediaPrivate;

#define BIT_RATE_BUF_SIZE   16
#define FRAME_RATE_BUF_SIZE 16

G_DEFINE_TYPE_WITH_PRIVATE(IpcamIMedia, ipcam_imedia, IPCAM_BASE_APP_TYPE)

static void ipcam_imedia_before(IpcamIMedia *imedia);
static void ipcam_imedia_in_loop(IpcamIMedia *imedia);
static void message_handler(GObject *obj, IpcamMessage* msg, gboolean timeout);
static void ipcam_imedia_query_day_night_mode(IpcamIMedia *imedia);
static void ipcam_imedia_query_osd_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_query_szyc_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_query_baseinfo_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_request_users(IpcamIMedia *imedia);
static void ipcam_imedia_request_rtsp_port(IpcamIMedia *imedia);
static void ipcam_imedia_query_rtsp_auth(IpcamIMedia *imedia);
static void ipcam_imedia_query_video_param(IpcamIMedia *imedia);
static void ipcam_imedia_set_users(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_set_rtsp_port(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_set_rtsp_auth(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_osd_display_video_data(GObject *obj);

static void ipcam_imedia_finalize(GObject *object)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(object));
    int i;

    g_clear_object(&priv->rtsp);
    g_clear_object(&priv->sys_ctrl);
    g_clear_object(&priv->video);
    for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++) {
        g_clear_object(&priv->osd[i]);
        g_free(priv->bit_rate[i]);
        g_free(priv->frame_rate[i]);
    }
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
	media_ircut_free(priv->ircut);
    G_OBJECT_CLASS(ipcam_imedia_parent_class)->finalize(object);
}
static void ipcam_imedia_init(IpcamIMedia *self)
{
	IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(self);
    const gchar *pattern = "InsBr.*InsFr[^\n]*[\n]+([^\n]*)[\n]([^\n]*)[\n]";
    int i;

    regcomp(&priv->reg, pattern, REG_EXTENDED | REG_NEWLINE);
    priv->sys_ctrl = g_object_new(IPCAM_MEDIA_SYS_CTRL_TYPE, NULL);
    priv->video = g_object_new(IPCAM_MEDIA_VIDEO_TYPE, "app", self, NULL);
    for (i = 0; i < STREAM_CHN_LAST; i++) {
        priv->bit_rate[i] = g_malloc(BIT_RATE_BUF_SIZE);
        priv->frame_rate[i] = g_malloc(FRAME_RATE_BUF_SIZE);
    }
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
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_video", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_osd", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_image", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_szyc", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(self), "set_day_night_mode", IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE);
    priv->osd_buffer = g_malloc(OSD_BUFFER_LENGTH);
	priv->ircut = media_ircut_new(256, 15);
	media_ircut_initialize(priv->ircut);
}
static void ipcam_imedia_class_init(IpcamIMediaClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_imedia_finalize;

    IpcamBaseServiceClass *base_service_class = IPCAM_BASE_SERVICE_CLASS(klass);
    base_service_class->before = &ipcam_imedia_before;
    base_service_class->in_loop = &ipcam_imedia_in_loop;
}
static void ipcam_imedia_before(IpcamIMedia *imedia)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    ipcam_imedia_sys_ctrl_init(priv->sys_ctrl);
    ipcam_imedia_request_users(imedia);
    ipcam_imedia_query_rtsp_auth(imedia);
    ipcam_imedia_request_rtsp_port(imedia);
    ipcam_imedia_query_video_param(imedia);
    priv->rtsp = g_object_new(IPCAM_TYPE_RTSP, NULL);
    ipcam_imedia_query_szyc_parameter(imedia);
    ipcam_imedia_query_osd_parameter(imedia);
    ipcam_imedia_query_baseinfo_parameter(imedia);
	ipcam_imedia_query_day_night_mode(imedia);
}
static void ipcam_imedia_in_loop(IpcamIMedia *imedia)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    time_t now;
    int i;
	gchar osdBuf[128] = {0};
    gchar timeBuf[64] = {0};

	if (media_ircut_detect(priv->ircut)) {
		gboolean ir_status = media_ircut_get_status(priv->ircut);

		ipcam_media_video_set_color2grey(priv->video, priv->stream_desc, ir_status);
	}

    time(&now);
    if (priv->last_time != now)
    {
        priv->last_time = now;
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S ", localtime(&now));

		gchar *project = (gchar *)ipcam_base_app_get_config(IPCAM_BASE_APP(imedia), "imedia_rtsp:project");

		if (!project) {
			g_strlcat(osdBuf, timeBuf, sizeof(osdBuf));
		}
		else {
			if (strcasecmp(project, "dctx") == 0) {
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
				g_strlcat(osdBuf, " ", sizeof(osdBuf));
			}
			else if (strcasecmp(project, "dttx") == 0) {
				if (priv->train_num) {
					g_strlcpy(osdBuf, "", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->train_num, sizeof(osdBuf));
				}
				if (priv->position_num) {
					g_strlcat(osdBuf, " ", sizeof(osdBuf));
					g_strlcat(osdBuf, priv->position_num, sizeof(osdBuf));
				}
				g_strlcat(osdBuf, " ", sizeof(osdBuf));
				g_strlcat(osdBuf, timeBuf, sizeof(osdBuf));
			}
		}

        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
        {
            if (!priv->osd[i])
                continue;
            ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_DATETIME, osdBuf);
            ipcam_iosd_invalidate(priv->osd[i]);
        }
    }
}
static void message_handler(GObject *obj, IpcamMessage* msg, gboolean timeout)
{
    g_return_if_fail(IPCAM_IS_IMEDIA(obj));

    if (!timeout && msg)
    {
#if 0
        gchar *str = ipcam_message_to_string(msg);
        g_print("result=\n%s\n", str);
        g_free(str);
#endif
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
                                message_handler, 10);
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
static IPCAM_OSD_TYPE ipcam_imedia_parse_osd_type(IpcamIMedia *imedia, const gchar *osd_name)
{
    IPCAM_OSD_TYPE type = IPCAM_OSD_TYPE_LAST;
    
    if (g_str_equal(osd_name, "datetime"))
    {
        type = IPCAM_OSD_TYPE_DATETIME;
    }
    else if (g_str_equal(osd_name, "device_name"))
    {
        type = IPCAM_OSD_TYPE_DEVICE_NAME;
    }
    else if (g_str_equal(osd_name, "comment"))
    {
        type = IPCAM_OSD_TYPE_COMMENT;
    }
    else if (g_str_equal(osd_name, "frame_rate"))
    {
        type = IPCAM_OSD_TYPE_FRAMERATE;
    }
    else if (g_str_equal(osd_name, "bit_rate"))
    {
        type = IPCAM_OSD_TYPE_BITRATE;
    }
    else if (g_str_equal(osd_name, "train_num"))
    {
        type = IPCAM_OSD_TYPE_TRAIN_NUM;
    }
    else if (g_str_equal(osd_name, "carriage_num"))
    {
        type = IPCAM_OSD_TYPE_CARRIAGE_NUM;
    }
    else if (g_str_equal(osd_name, "position_num"))
    {
        type = IPCAM_OSD_TYPE_POSITION_NUM;
    }
    else
    {
        g_warn_if_reached();
    }

    return type;
}

void ipcam_imedia_got_osd_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    IpcamOSDParameter parameter;
    JsonObject *profile_object;
    JsonObject *osd_object;
    JsonObject *res_object;
    JsonObject *color_object;
    GList *members, *item;
    IPCAM_OSD_TYPE type = IPCAM_OSD_TYPE_LAST;
    const char *key[] = {"master", "slave"};
    gint i = 0;

    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    for (i = 0; i < ARRAY_SIZE(key); i++)
    {
        if (json_object_has_member(res_object, key[i]))
        {
            profile_object = json_object_get_object_member(res_object, key[i]);
            members = json_object_get_members(profile_object);            
            for (item = g_list_first(members); item; item = g_list_next(item))
            {
                const gchar *name = item->data;

                osd_object = json_object_get_object_member(profile_object, name);
                parameter.is_show = json_object_get_boolean_member(osd_object, "isshow");
                parameter.font_size = json_object_get_int_member(osd_object, "size");
                parameter.position.x = json_object_get_int_member(osd_object, "left");
                parameter.position.y = json_object_get_int_member(osd_object, "top");
                color_object = json_object_get_object_member(osd_object, "color");
                if (color_object)
                {
                    parameter.color.red = json_object_get_int_member(color_object, "red");
                    parameter.color.green = json_object_get_int_member(color_object, "green");
                    parameter.color.blue = json_object_get_int_member(color_object, "blue");
                    parameter.color.alpha = json_object_get_int_member(color_object, "alpha");
                }
                
                type = ipcam_imedia_parse_osd_type(imedia, name);
                if (type >= 0 && type < IPCAM_OSD_TYPE_LAST)
                {
                    ipcam_iosd_start(priv->osd[i], type, &parameter);
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
    int i;

    items_obj = json_object_get_object_member(json_node_get_object(body), "items");

    if (json_object_has_member(items_obj, "night_mode_threshold"))
    {
        threshold = json_object_get_int_member(items_obj, "night_mode_threshold");

		media_ircut_set_sensitivity(priv->ircut, threshold);
    }
    if (json_object_has_member(items_obj, "ir_intensity"))
    {
        ir_intensity = json_object_get_int_member(items_obj, "ir_intensity");

		media_ircut_set_ir_intensity(priv->ircut, ir_intensity);
	}
}

void ipcam_imedia_got_szyc_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *items_obj;
    gchar *train_num = NULL;
    gchar *carriage_num = NULL;
    gchar *position_num = NULL;
    int i;

    items_obj = json_object_get_object_member(json_node_get_object(body), "items");

    if (json_object_has_member(items_obj, "train_num"))
    {
        train_num = json_object_get_string_member(items_obj, "train_num");
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
            ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_TRAIN_NUM, train_num);

		g_free(priv->train_num);
		priv->train_num = g_strdup(train_num);
    }
    if (json_object_has_member(items_obj, "carriage_num"))
    {
        carriage_num = json_object_get_string_member(items_obj, "carriage_num");
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
            ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_CARRIAGE_NUM, carriage_num);

		g_free(priv->carriage_num);
		priv->carriage_num = g_strdup(carriage_num);
	}
    if (json_object_has_member(items_obj, "position_num"))
    {
        position_num = json_object_get_string_member(items_obj, "position_num");
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
            ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_POSITION_NUM, position_num);

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
        ipcam_ivideo_param_change(priv->video, priv->stream_desc);
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

    if (NULL != device_name && strlen(device_name) > 0)
    {
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
            ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_DEVICE_NAME, device_name);
    }

    const gchar *comment = NULL;
    comment = json_object_get_string_member(res_object, "comment");
    if (NULL != comment && strlen(comment) > 0)
    {
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
            ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_COMMENT, comment);
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
        char *username = NULL;
        char *password = NULL;

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
        char *username;

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
        priv->stream_desc[chn].v_desc.path = g_strdup(json_object_get_string_member(obj, "stream_path"));
        ipcam_rtsp_set_stream_path(priv->rtsp, chn, priv->stream_desc[chn].v_desc.path);
    }
}

void ipcam_imedia_got_video_param(IpcamIMedia *imedia, JsonNode *body, gboolean is_notice)
{
    JsonObject *res_obj = json_object_get_object_member(json_node_get_object(body), "items");
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
	const gchar *font = ipcam_base_app_get_config(IPCAM_BASE_APP(imedia), "imedia_rtsp:font");

    if (json_object_has_member(res_obj, "profile"))
    {
        gchar *profile = json_object_get_string_member(res_obj, "profile");
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

        ipcam_ivideo_start(priv->video, priv->stream_desc);
        for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
        {
            priv->osd[i] = g_object_new(IPCAM_MEDIA_OSD_TYPE,
                                        "RgnHandle", i,
                                        "VencGroup", i,
										"font", font,
                                        NULL);
            ipcam_media_osd_set_image_size(priv->osd[i],
                                           priv->stream_desc[i].v_desc.image_width,
                                           priv->stream_desc[i].v_desc.image_height);
        }
        ipcam_base_app_add_timer(IPCAM_BASE_APP(imedia), "osd_display_video_data", "1",
                                 ipcam_imedia_osd_display_video_data);
        ipcam_rtsp_set_video_iface(priv->rtsp, priv->video);
        ipcam_rtsp_start_server(priv->rtsp);
    }
    else
    {
        int i;

        for (i = 0; i < STREAM_CHN_LAST; i++) {
            ipcam_media_osd_set_image_size(priv->osd[i],
                                           priv->stream_desc[i].v_desc.image_width,
                                           priv->stream_desc[i].v_desc.image_height);
        }
        ipcam_ivideo_param_change(priv->video, priv->stream_desc);
    }
}

static void ipcam_imedia_osd_int_to_string(IpcamIMedia *imedia,
                                           gint val,
                                           gchar **string,
                                           gint len,
                                           gchar *extra)
{
    memset(*string, 0, len);
    sprintf(*string, "%d%s", val, extra);
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
                if (id >= 0 && id < STREAM_CHN_LAST) {
                    ipcam_imedia_osd_int_to_string(imedia, bit_rate_val, &priv->bit_rate[id], BIT_RATE_BUF_SIZE, "kbps");
                    ipcam_imedia_osd_int_to_string(imedia, frame_rate_val, &priv->frame_rate[id], FRAME_RATE_BUF_SIZE, "fps");
                }
            }
        }
    }
}
static void ipcam_imedia_osd_display_video_data(GObject *obj)
{
    g_return_if_fail(IPCAM_IS_IMEDIA(obj));
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(obj));
    int i;

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

            for (i = MASTER_CHN; i < STREAM_CHN_LAST; i++)
            {
                ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_FRAMERATE, priv->frame_rate[i]);
                ipcam_iosd_set_content(priv->osd[i], IPCAM_OSD_TYPE_BITRATE, priv->bit_rate[i]);
            }
        }
    }
}

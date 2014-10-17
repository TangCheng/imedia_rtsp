#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include <messages.h>
#include <json-glib/json-glib.h>
#include "imedia.h"
#include "media_sys_ctrl_interface.h"
#include "media_video_interface.h"
#include "media_osd_interface.h"
#include "stream_descriptor.h"
#if defined(HI3518) || defined(HI3516)
#include "hi3518/media_sys_ctrl.h"
#include "hi3518/media_video.h"
#include "hi3518/media_osd.h"
#endif
#include "rtsp/rtsp.h"

typedef struct _IpcamIMediaPrivate
{
    IpcamIMediaSysCtrl *sys_ctrl;
    IpcamIVideo *video;
    IpcamIOSD *osd;
    IpcamRtsp *rtsp;
    time_t last_time;
    regex_t reg;
    gchar *bit_rate;
    gchar *frame_rate;
} IpcamIMediaPrivate;

#define BIT_RATE_BUF_SIZE   16
#define FRAME_RATE_BUF_SIZE 16

G_DEFINE_TYPE_WITH_PRIVATE(IpcamIMedia, ipcam_imedia, IPCAM_BASE_APP_TYPE)

static void ipcam_imedia_before(IpcamIMedia *imedia);
static void ipcam_imedia_in_loop(IpcamIMedia *imedia);
static void message_handler(GObject *obj, IpcamMessage* msg, gboolean timeout);
static void ipcam_imedia_query_osd_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_query_baseinfo_parameter(IpcamIMedia *imedia);
static void ipcam_imedia_request_users(IpcamIMedia *imedia);
static void ipcam_imedia_request_rtsp_port(IpcamIMedia *imedia);
static void ipcam_imedia_query_rtsp_auth(IpcamIMedia *imedia);
static void ipcam_imedia_got_osd_parameter(IpcamIMedia *imedia, JsonNode *body);
static void ipcam_imedia_got_baseinfo_parameter(IpcamIMedia *imedia, JsonNode *body);
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
    g_clear_object(&priv->osd);
    regfree(&priv->reg);
    g_free(priv->bit_rate);
    g_free(priv->frame_rate);
    G_OBJECT_CLASS(ipcam_imedia_parent_class)->finalize(object);
}
static void ipcam_imedia_init(IpcamIMedia *self)
{
	IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(self);
    const gchar *pattern = "InsBr.*InsFr.*[\n]+";
    regcomp(&priv->reg, pattern, REG_EXTENDED | REG_NEWLINE);
    priv->sys_ctrl = g_object_new(IPCAM_MEDIA_SYS_CTRL_TYPE, NULL);
    priv->video = g_object_new(IPCAM_MEDIA_VIDEO_TYPE, NULL);
    priv->bit_rate = g_malloc(BIT_RATE_BUF_SIZE);
    priv->frame_rate = g_malloc(FRAME_RATE_BUF_SIZE);
    time(&priv->last_time);
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
    ipcam_ivideo_start(priv->video);
    priv->osd = g_object_new(IPCAM_MEDIA_OSD_TYPE, NULL);
    ipcam_imedia_query_osd_parameter(imedia);
    ipcam_imedia_query_baseinfo_parameter(imedia);
    ipcam_imedia_request_users(imedia);
    ipcam_imedia_request_rtsp_port(imedia);
    ipcam_imedia_query_rtsp_auth(imedia);
    ipcam_base_app_add_timer(IPCAM_BASE_APP(imedia), "osd_display_video_data", "1", ipcam_imedia_osd_display_video_data);
    priv->rtsp = g_object_new(IPCAM_TYPE_RTSP, NULL);
}
static void ipcam_imedia_in_loop(IpcamIMedia *imedia)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    time_t now;
    gchar timeBuf[24] = {0};

    time(&now);
    if (priv->last_time != now)
    {
        priv->last_time = now;
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S ", localtime(&now));
        ipcam_iosd_set_content(priv->osd, IPCAM_OSD_TYPE_DATETIME, timeBuf);
        ipcam_iosd_invalidate(priv->osd);
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

        if (g_str_equal(action, "get_osd"))
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
        else
        {
            g_warn_if_reached();
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
    else
    {
        g_warn_if_reached();
    }

    return type;
}

static void ipcam_imedia_got_osd_parameter(IpcamIMedia *imedia, JsonNode *body)
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
                parameter.position.x = json_object_get_int_member(osd_object, "left") * (guint32)IMAGE_WIDTH / 1000;
                parameter.position.y = json_object_get_int_member(osd_object, "top") * (guint32)IMAGE_HEIGHT / 1000;
                color_object = json_object_get_object_member(osd_object, "color");
                if (color_object)
                {
                    parameter.color.red = json_object_get_int_member(color_object, "red");
                    parameter.color.green = json_object_get_int_member(color_object, "green");
                    parameter.color.blue = json_object_get_int_member(color_object, "blue");
                    parameter.color.alpha = json_object_get_int_member(color_object, "alpha");
                }
                
                type = ipcam_imedia_parse_osd_type(imedia, name);
                if (type != IPCAM_OSD_TYPE_LAST)
                {
                    ipcam_iosd_start(priv->osd, type, &parameter);
                }
            }
            g_list_free(members);
        }
    }
}
static void ipcam_imedia_got_baseinfo_parameter(IpcamIMedia *imedia, JsonNode *body)
{
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    JsonObject *res_object;
    
    res_object = json_object_get_object_member(json_node_get_object(body), "items");

    const gchar *device_name = NULL;
    device_name = json_object_get_string_member(res_object, "device_name");
    g_print("%s\n", device_name);
    if (NULL != device_name && strlen(device_name) > 0)
    {
        ipcam_iosd_set_content(priv->osd, IPCAM_OSD_TYPE_DEVICE_NAME, device_name);
    }

    const gchar *comment = NULL;
    comment = json_object_get_string_member(res_object, "comment");
    if (NULL != comment && strlen(comment) > 0)
    {
        ipcam_iosd_set_content(priv->osd, IPCAM_OSD_TYPE_COMMENT, comment);
    }
}


static void ipcam_imedia_set_rtsp_port(IpcamIMedia *imedia, JsonNode *body)
{
    JsonObject *items = json_object_get_object_member(json_node_get_object(body), "items");
    JsonObject *server_port = json_object_get_object_member(items, "port");
    const guint rtsp_port = json_object_get_int_member(server_port, "rtsp");
    g_print("rtsp_port = %u\n", rtsp_port);
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    ipcam_rtsp_set_port(priv->rtsp, rtsp_port);
}

static void proc_each_user(JsonArray *array, guint index_, JsonNode *element_node, gpointer user_data)
{
    const gchar *username = json_object_get_string_member(json_node_get_object(element_node), "username");
    const gchar *password = json_object_get_string_member(json_node_get_object(element_node), "password");
    g_print("username = %s\npassword = %s\n", username, password);
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(user_data));
    ipcam_rtsp_insert_user(priv->rtsp, username, password);
}

static void ipcam_imedia_set_users(IpcamIMedia *irtsp, JsonNode *body)
{
    JsonArray *users = json_object_get_array_member(json_node_get_object(body), "items");
    json_array_foreach_element(users, proc_each_user, irtsp);
}

static void ipcam_imedia_set_rtsp_auth(IpcamIMedia *imedia, JsonNode *body)
{
    JsonObject *res_obj = json_object_get_object_member(json_node_get_object(body), "items");
    gboolean rtsp_auth = json_object_get_boolean_member(res_obj, "rtsp_auth");
    g_print("rtsp_auth = %d\n", rtsp_auth);
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);
    ipcam_rtsp_set_auth(priv->rtsp, rtsp_auth);
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
    const size_t nmatch = 1;
    regmatch_t pmatch[1];
    gint status;
    gint i;
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(imedia);

    status = regexec(&priv->reg, buffer, nmatch, pmatch, 0);
    if (REG_NOMATCH != status)
    {
        gchar *p = buffer + pmatch[0].rm_eo;
        gint id;
        gint bit_rate_val;
        gint frame_rate_val;
        sscanf(p, "%d%d%d", &id, &bit_rate_val, &frame_rate_val);
        /*
        for (gint i = pmatch[0].rm_so; i < pmatch[0].rm_eo; i++)
        {
            g_print("%c", buffer[i]);
        }
        g_print("\n");
        */
        //g_print("%d %d %d\n", id, bit_rate_val, frame_rate_val);
        ipcam_imedia_osd_int_to_string(imedia, bit_rate_val, &priv->bit_rate, BIT_RATE_BUF_SIZE, "kbps");
        ipcam_imedia_osd_int_to_string(imedia, frame_rate_val, &priv->frame_rate, FRAME_RATE_BUF_SIZE, "fps");
    }
}
static void ipcam_imedia_osd_display_video_data(GObject *obj)
{
    g_return_if_fail(IPCAM_IS_IMEDIA(obj));
    IpcamIMediaPrivate *priv = ipcam_imedia_get_instance_private(IPCAM_IMEDIA(obj));

    FILE *rc_file = fopen("/proc/umap/rc", "r");
    if (NULL != rc_file)
    {
        gchar *buffer = g_malloc0(4096);
        if (fread(buffer, 1, 4096, rc_file) > 0)
        {
            ipcam_imedia_osd_lookup_video_run_info(IPCAM_IMEDIA(obj), buffer);
        }        
        fclose(rc_file);
        g_free(buffer);

        ipcam_iosd_set_content(priv->osd, IPCAM_OSD_TYPE_FRAMERATE, priv->frame_rate);
        ipcam_iosd_set_content(priv->osd, IPCAM_OSD_TYPE_BITRATE, priv->bit_rate);
    }
}

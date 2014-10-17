#include "rtsp.h"
#include "messages.h"
#include <json-glib/json-glib.h>

enum
{
    PROP_0,
    PROP_FD,
    N_PROPERTIES
};

typedef struct _IpcamRtspPrivate
{
    gint32 venc_fd;
    guint rtsp_port;
    GHashTable *users_hash;
    GThread *live555_thread;
    gchar watch_variable;
} IpcamRtspPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamRtsp, ipcam_rtsp, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

extern void launch_rtsp_server(unsigned int port, char *watchVariable);
static void ipcam_rtsp_before_start(IpcamRtsp *rtsp);
static void ipcam_rtsp_in_loop(IpcamRtsp *rtsp);
static void message_handler(GObject *obj, IpcamMessage* msg, gboolean timeout);
static void ipcam_rtsp_request_users(IpcamRtsp *rtsp);
static void ipcam_rtsp_request_rtsp_port(IpcamRtsp *rtsp);
static void ipcam_rtsp_set_rtsp_port(IpcamRtsp *rtsp, JsonNode *body);
static void ipcam_rtsp_set_users(IpcamRtsp *rtsp, JsonNode *body);

static gpointer live555_thread_proc(gpointer data)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)data;
    launch_rtsp_server(priv->rtsp_port, &priv->watch_variable);
    g_thread_exit(0);
    return NULL;
}

static void ipcam_rtsp_finalize(GObject *self)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(IPCAM_RTSP(self));
    priv->watch_variable = 1;
    g_thread_join(priv->live555_thread);
    g_hash_table_destroy(priv->users_hash);
    G_OBJECT_CLASS(ipcam_rtsp_parent_class)->finalize(self);
}

static void
destroy_notify(gpointer data)
{
    g_free(data);
}

static void
ipcam_rtsp_init(IpcamRtsp *self)
{
	IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(self);
    priv->rtsp_port = 8554;
    priv->users_hash = g_hash_table_new_full(g_str_hash, g_str_equal, destroy_notify, destroy_notify);
    priv->watch_variable = 0;
    priv->live555_thread = g_thread_new("live555", live555_thread_proc, priv);
}

static void ipcam_rtsp_get_property(GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    IpcamRtsp *self = IPCAM_RTSP(object);
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(self);
    switch(property_id)
    {
    case PROP_FD:
        {
            g_value_set_int(value, priv->venc_fd);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
static void ipcam_rtsp_set_property(GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
    IpcamRtsp *self = IPCAM_RTSP(object);
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(self);
    switch(property_id)
    {
    case PROP_FD:
        {
            priv->venc_fd = g_value_get_int(value);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void ipcam_rtsp_class_init(IpcamRtspClass *klass)
{

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_rtsp_finalize;
    object_class->get_property = &ipcam_rtsp_get_property;
    object_class->set_property = &ipcam_rtsp_set_property;

    obj_properties[PROP_FD] =
        g_param_spec_int("venc_fd",
                         "venc fd",
                         "venc fd.",
                         G_MININT,
                         G_MAXINT,
                         -1, // default value
                         G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}


#include "rtsp.h"
#include "messages.h"
#include <json-glib/json-glib.h>

typedef struct _IpcamRtspPrivate
{
    guint port;
    GHashTable *users_hash;
    gboolean auth;
    GThread *live555_thread;
    gchar watch_variable;
} IpcamRtspPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamRtsp, ipcam_rtsp, G_TYPE_OBJECT)

extern void launch_rtsp_server(unsigned int port, char *watchVariable);

static gpointer
live555_thread_proc(gpointer data)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)data;
    launch_rtsp_server(priv->port, &priv->watch_variable);
    g_thread_exit(0);
    return NULL;
}

static void
ipcam_rtsp_finalize(GObject *self)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(IPCAM_RTSP(self));
    priv->watch_variable = 1;
    g_thread_join(priv->live555_thread);
    g_hash_table_destroy(priv->users_hash);
    G_OBJECT_CLASS(ipcam_rtsp_parent_class)->finalize(self);
}

static void
ipcam_rtsp_init(IpcamRtsp *self)
{
	IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(self);
    priv->port = 554;
    priv->users_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    priv->watch_variable = 0;
    priv->live555_thread = g_thread_new("live555", live555_thread_proc, priv);
}

static void
ipcam_rtsp_class_init(IpcamRtspClass *klass)
{

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_rtsp_finalize;
}

void ipcam_rtsp_set_port(IpcamRtsp *rtsp, guint port)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    priv->port = port;
}

void ipcam_rtsp_insert_user(IpcamRtsp *rtsp, const gchar *username, const gchar *password)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    g_hash_table_insert(priv->users_hash, g_strdup(username), g_strdup(password));
}

void ipcam_rtsp_set_auth(IpcamRtsp *rtsp, gboolean auth)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    priv->auth = auth;
}

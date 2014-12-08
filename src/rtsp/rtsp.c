#include "rtsp.h"
#include "messages.h"
#include "live/iRTSPServerLauncher.hh"
#include <json-glib/json-glib.h>

typedef struct _IpcamRtspPrivate
{
    guint port;
    GHashTable *users_hash;
    gboolean auth;
    GThread *live555_thread;
    gchar watch_variable;
    const gchar *stream_path[STREAM_CHN_LAST];
    IpcamIVideo *video;
} IpcamRtspPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamRtsp, ipcam_rtsp, G_TYPE_OBJECT)

static gpointer
live555_thread_proc(gpointer data)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)data;
    rtsp_user *users = NULL;
    gssize len = 0;
    if (priv->auth)
    {
        len = g_hash_table_size(priv->users_hash);
        users = g_new0(rtsp_user, len);
        guint i = 0;
        GHashTableIter iter;
        gpointer key, value;

        g_hash_table_iter_init (&iter, priv->users_hash);
        while (g_hash_table_iter_next (&iter, &key, &value))
        {
            users[i].name = (gchar *)key;
            users[i].password = (gchar *)value;
            i++;
        }
    }
    launch_rtsp_server(priv->video,
                       priv->port,
                       &priv->watch_variable,
                       priv->auth,
                       users, len,
                       priv->stream_path, STREAM_CHN_LAST);
    g_free(users);
    g_thread_exit(0);
    return NULL;
}

static void
ipcam_rtsp_finalize(GObject *self)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(IPCAM_RTSP(self));
    ipcam_rtsp_stop_server(IPCAM_RTSP(self));
    g_hash_table_destroy(priv->users_hash);
    G_OBJECT_CLASS(ipcam_rtsp_parent_class)->finalize(self);
}

static void
ipcam_rtsp_init(IpcamRtsp *self)
{
	IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(self);
    priv->port = 554;
    priv->users_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    priv->stream_path[MASTER] = NULL;
    priv->stream_path[SLAVE] = NULL;
    priv->video = NULL;
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

void ipcam_rtsp_set_stream_path(IpcamRtsp *rtsp, enum StreamChannel chn, const gchar *path)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    priv->stream_path[chn] = path;
}

void ipcam_rtsp_set_video_iface(IpcamRtsp *rtsp, IpcamIVideo *video)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    priv->video = video;
}

void ipcam_rtsp_start_server(IpcamRtsp *rtsp)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    priv->watch_variable = 0;
    priv->live555_thread = g_thread_new("live555", live555_thread_proc, priv);
}

void ipcam_rtsp_stop_server(IpcamRtsp *rtsp)
{
    IpcamRtspPrivate *priv = ipcam_rtsp_get_instance_private(rtsp);
    priv->watch_variable = 1;
    g_thread_join(priv->live555_thread);
}

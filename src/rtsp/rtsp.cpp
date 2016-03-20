#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "messages.h"
#include "imedia.h"
#include "live/LiveStreamInput.hh"
#include "live/LiveStreamRTSPServer.hh"
#include "rtsp.h"

typedef struct _IpcamRtspPrivate
{
    guint port;
    GThread *live555_thread;
    gchar watch_variable;
    StreamDescriptor *stream_desc[STREAM_CHN_LAST];
    gpointer user_data;
    gboolean enable_auth;
    UserAuthenticationDatabase *auth_db;
    LiveStreamRTSPServer *rtsp_server;
    TaskScheduler* m_scheduler;
    UsageEnvironment* m_env;
} IpcamRtspPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamRtsp, ipcam_rtsp, G_TYPE_OBJECT)

static gpointer
live555_thread_proc(gpointer data)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)data;
    UserAuthenticationDatabase *auth_db;

    OutPacketBuffer::increaseMaxSizeTo(2 * 1024 * 1024);

    auth_db = priv->enable_auth ? priv->auth_db : NULL;
    priv->rtsp_server = LiveStreamRTSPServer::createNew(*priv->m_env, priv->port, auth_db);
    if (priv->rtsp_server == NULL) {
        *priv->m_env << "Failed to create RTSP server: "
            << priv->m_env->getResultMsg() << "\n";
        return NULL;
    }

    for (int i = 0; i < STREAM_CHN_LAST; i++)
    {
        LiveStreamInput *streamInput;

        streamInput = LiveStreamInput::createNew(*priv->m_env, (StreamChannel)i, priv->stream_desc[i]);

        priv->rtsp_server->addStreamInput(streamInput);
    }

    priv->m_env->taskScheduler().doEventLoop(&priv->watch_variable); // does not return

    RTSPServer::close(priv->rtsp_server);
    priv->rtsp_server = NULL;

    return NULL;
}

static void
ipcam_rtsp_finalize(GObject *self)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(IPCAM_RTSP(self));
    ipcam_rtsp_stop_server(IPCAM_RTSP(self));

    delete priv->auth_db;

    if (priv->m_env)
    {
        priv->m_env->reclaim();
        priv->m_env = NULL;
    }
    if (priv->m_scheduler)
    {
        delete priv->m_scheduler;
        priv->m_scheduler = NULL;
    }
    G_OBJECT_CLASS(ipcam_rtsp_parent_class)->finalize(self);
}

static void
ipcam_rtsp_init(IpcamRtsp *self)
{
	IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(self);

    priv->port = 554;
    priv->stream_desc[MASTER] = NULL;
    priv->stream_desc[SLAVE] = NULL;
    priv->user_data = NULL;

    priv->auth_db = new UserAuthenticationDatabase;

    priv->m_scheduler = BasicTaskScheduler::createNew();
    priv->m_env = BasicUsageEnvironment::createNew(*priv->m_scheduler);
}

static void
ipcam_rtsp_class_init(IpcamRtspClass *klass)
{

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_rtsp_finalize;
}

void ipcam_rtsp_set_port(IpcamRtsp *rtsp, guint port)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->port = port;
}

void ipcam_rtsp_insert_user(IpcamRtsp *rtsp, const gchar *username, const gchar *password)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);

    if (priv->auth_db->lookupPassword(username))
        priv->auth_db->removeUserRecord(username);

    priv->auth_db->addUserRecord(username, password);
}

void ipcam_rtsp_delete_user(IpcamRtsp *rtsp, const gchar *username)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);

    priv->auth_db->removeUserRecord(username);
}

void ipcam_rtsp_set_auth(IpcamRtsp *rtsp, gboolean auth)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->enable_auth = auth;

    if (priv->rtsp_server) {
        UserAuthenticationDatabase *auth_db;

        auth_db = priv->enable_auth ? priv->auth_db : NULL;

        priv->rtsp_server->setAuthenticationDatabase(auth_db);
    }
}

void ipcam_rtsp_set_stream_desc(IpcamRtsp *rtsp, StreamChannel chn, StreamDescriptor *desc)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->stream_desc[chn] = desc;
}

void ipcam_rtsp_set_user_data(IpcamRtsp *rtsp, gpointer user_data)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->user_data = user_data;
}

void ipcam_rtsp_start_server(IpcamRtsp *rtsp)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->watch_variable = 0;
    priv->live555_thread = g_thread_new("live555", live555_thread_proc, priv);
}

void ipcam_rtsp_stop_server(IpcamRtsp *rtsp)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->watch_variable = 1;
    g_thread_join(priv->live555_thread);
}

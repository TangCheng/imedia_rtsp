#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "rtsp.h"
#include "messages.h"
#include "live/H264LiveStreamServerMediaSubsession.hh"
#include "live/H264LiveStreamInput.hh"
#include "live/H264LiveStreamSource.hh"

typedef struct _IpcamRtspPrivate
{
    guint port;
    GThread *live555_thread;
    gchar watch_variable;
    const gchar *stream_path[STREAM_CHN_LAST];
    IpcamMediaVideo *video;
    gboolean enable_auth;
    UserAuthenticationDatabase *auth_db;
    RTSPServer *rtsp_server;
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
    priv->rtsp_server = RTSPServer::createNew(*priv->m_env, priv->port, auth_db);
    if (priv->rtsp_server == NULL) {
        *priv->m_env << "Failed to create RTSP server: "
            << priv->m_env->getResultMsg() << "\n";
        return NULL;
    }

    ServerMediaSession* sms[STREAM_CHN_LAST];

    for (int i = 0; i < STREAM_CHN_LAST; i++)
    {
        char const* streamName = priv->stream_path[i];
        char const* descriptionString = "Session streamed by \"iRTSP\"";
        H264LiveStreamInput *inputDevice = H264LiveStreamInput::createNew(*priv->m_env, priv->video, (StreamChannel)i);
        ServerMediaSession* sms =
            ServerMediaSession::createNew(*priv->m_env, streamName, streamName, descriptionString);
        sms->addSubsession(H264LiveStreamServerMediaSubsession::createNew(*priv->m_env, *inputDevice));
        priv->rtsp_server->addServerMediaSession(sms);
    }

    priv->m_env->taskScheduler().doEventLoop(&priv->watch_variable); // does not return

    for (int i = 0; i < STREAM_CHN_LAST; i++)
    {
        priv->rtsp_server->deleteServerMediaSession(sms[i]);
    }
    
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
    priv->stream_path[MASTER] = NULL;
    priv->stream_path[SLAVE] = NULL;
    priv->video = NULL;

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

void ipcam_rtsp_set_stream_path(IpcamRtsp *rtsp, enum StreamChannel chn, const gchar *path)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->stream_path[chn] = path;
}

void ipcam_rtsp_set_video_iface(IpcamRtsp *rtsp, IpcamMediaVideo *video)
{
    IpcamRtspPrivate *priv = (IpcamRtspPrivate *)ipcam_rtsp_get_instance_private(rtsp);
    priv->video = video;
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

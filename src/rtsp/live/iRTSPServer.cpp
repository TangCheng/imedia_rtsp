#include "iRTSPServer.hh"

IRTSPServer::IRTSPServer(void *videoEngine, Boolean iFramesOnly)
     : m_VideoEngine(videoEngine),
       m_iFramesOnly(iFramesOnly),
       m_authDB(NULL),
       m_rtspServer(NULL)
{
    // Begin by setting up our usage environment:
    m_scheduler = BasicTaskScheduler::createNew();
    m_env = BasicUsageEnvironment::createNew(*m_scheduler);
}

IRTSPServer::~IRTSPServer()
{
    if (m_env)
    {
        m_env->reclaim();
        m_env = NULL;
    }
    if (m_scheduler)
    {
        delete m_scheduler;
        m_scheduler = NULL;
    }
    if (m_authDB)
    {
        delete m_authDB;
        m_authDB = NULL;
    }
}

void IRTSPServer::setAuthUser(user *users, size_t len)
{
    // To implement client access control to the RTSP server, do the following:
    if (m_authDB == NULL)
    {
        m_authDB = new UserAuthenticationDatabase;
    }
    
    if (m_authDB)
    {
        int i = 0;
        for (i = 0; i < len; i++)
        {
            m_authDB->addUserRecord(users[i].name, users[i].password);
        }
    }
}

void IRTSPServer::setPath(char **path, size_t len)
{
    m_path = path;
    m_pathSize = len;
}

void IRTSPServer::startServer(unsigned int port, char *watchVariable)
{
    OutPacketBuffer::increaseMaxSizeTo(1024 * 1024);
    m_rtspServer = RTSPServer::createNew(*m_env, port, m_authDB);
    if (m_rtspServer == NULL) {
        *m_env << "Failed to create RTSP server: " << m_env->getResultMsg() << "\n";
        return;
    }
    ServerMediaSession* sms[m_pathSize];
    
    for (int i = 0; i < m_pathSize; i++)
    {
        char const* streamName = m_path[i];
        char const* descriptionString = "Session streamed by \"iRTSP\"";
        H264LiveStreamInput *inputDevice = H264LiveStreamInput::createNew(*m_env, m_VideoEngine);
        ServerMediaSession* sms =
            ServerMediaSession::createNew(*m_env, streamName, streamName, descriptionString);
        sms->addSubsession(H264LiveStreamServerMediaSubsession::createNew(*m_env, *inputDevice));
        m_rtspServer->addServerMediaSession(sms);
    }

    m_env->taskScheduler().doEventLoop(watchVariable); // does not return
    for (int i = 0; i < m_pathSize; i++)
    {
        m_rtspServer->deleteServerMediaSession(sms[i]);
    }
    
    RTSPServer::close(m_rtspServer);
    m_rtspServer = NULL;
}

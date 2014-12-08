#ifndef _IRTSP_SERVER_HH
#define _IRTSP_SERVER_HH

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include "H264LiveStreamServerMediaSubsession.hh"
#include "H264LiveStreamInput.hh"
#include "H264LiveStreamSource.hh"
#include "iRTSPServerLauncher.hh"

class IRTSPServer
{
private:
    TaskScheduler* m_scheduler;
    UsageEnvironment* m_env;
    UserAuthenticationDatabase* m_authDB;
    char **m_path;
    size_t m_pathSize;
    RTSPServer* m_rtspServer;
    
    // To stream *only* MPEG-1 or 2 video "I" frames
    // (e.g., to reduce network bandwidth),
    // change the following "False" to "True":
    Boolean m_iFramesOnly;
    void *m_VideoEngine;
public:
    IRTSPServer(void *videoEngine, Boolean iFramesOnly = False);
    virtual ~IRTSPServer();
    void setAuthUser(rtsp_user *users, size_t len);
    void setPath(char *path[], size_t len);
    void startServer(unsigned int port, char *watchVariable);
};

#endif /* _RTSP_SERVER_HH */

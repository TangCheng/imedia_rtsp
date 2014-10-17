#ifndef _IRTSP_SERVER_HH
#define _IRTSP_SERVER_HH

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include "H264LiveStreamServerMediaSubsession.hh"
#include "H264LiveStreamSource.hh"

class IRTSPServer
{
private:
    TaskScheduler* m_scheduler;
    UsageEnvironment* m_env;
    UserAuthenticationDatabase* m_authDB;
    RTSPServer* m_rtspServer;
    // To make the second and subsequent client for each stream reuse the same
    // input stream as the first client (rather than playing the file from the
    // start for each client), change the following "False" to "True":
    Boolean m_reuseFirstSource;

    // To stream *only* MPEG-1 or 2 video "I" frames
    // (e.g., to reduce network bandwidth),
    // change the following "False" to "True":
    Boolean m_iFramesOnly;
public:
    IRTSPServer(Boolean reuseFirstSource = True, Boolean iFramesOnly = False);
    virtual ~IRTSPServer();
    void setAuthUser();
    void startServer(unsigned int port, char *watchVariable);
};

#endif /* _RTSP_SERVER_HH */

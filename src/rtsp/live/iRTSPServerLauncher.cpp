#include "iRTSPServer.hh"
#include "iRTSPServerLauncher.hh"

#ifdef __cplusplus
extern "C"
{
#endif

void launch_rtsp_server(void *video_engine,
                        unsigned int port,
                        char *watchVariable,
                        int auth,
                        user *users, size_t users_size,
                        char *path[], size_t path_size)
{
    IRTSPServer *server = new IRTSPServer(video_engine);
    if (auth != 0)
    {
        server->setAuthUser(users, users_size);
    }
    server->setPath(path, path_size);
    server->startServer(port, watchVariable);
}

void signalNewFrameData(void *clientData)
{
    TaskScheduler* ourScheduler = NULL; //%%% TO BE WRITTEN %%%
    H264LiveStreamSource* ourDevice  = (H264LiveStreamSource *)clientData; //%%% TO BE WRITTEN %%%

    if (ourDevice && ourDevice->getRefCount() != 0)
    { // sanity check
        ourScheduler = &(ourDevice->envir().taskScheduler());
        if (ourScheduler != NULL)
        {
            ourScheduler->triggerEvent(H264LiveStreamSource::eventTriggerId, ourDevice);
        }
    }
}

#ifdef __cplusplus
}
#endif

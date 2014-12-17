#include "stream_descriptor.h"
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
                        rtsp_user *users, size_t users_size,
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

#ifdef __cplusplus
}
#endif

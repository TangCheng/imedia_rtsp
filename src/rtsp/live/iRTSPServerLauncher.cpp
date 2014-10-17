#include "iRTSPServer.hh"

#ifdef __cplusplus
extern "C"
{
#endif

void launch_rtsp_server(unsigned int port, char *watchVariable)
{
    IRTSPServer *server = new IRTSPServer();
    server->startServer(port, watchVariable);
}

#ifdef __cplusplus
}
#endif

#ifndef __IRTSP_SERVER_LAUNCHER_HH__
#define __IRTSP_SERVER_LAUNCHER_HH__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _user
{
    char *name;
    char *password;
} user;

void launch_rtsp_server(void *video_engine,
                        unsigned int port,
                        char *watchVariable,
                        int auth,
                        user *users, size_t users_size,
                        char *path[], size_t path_size);

void signalNewFrameData(void *clientData);
     
#ifdef __cplusplus
}
#endif
     
#endif

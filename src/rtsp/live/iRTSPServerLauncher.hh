#ifndef __IRTSP_SERVER_LAUNCHER_HH__
#define __IRTSP_SERVER_LAUNCHER_HH__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _rtsp_user
{
    char *name;
    char *password;
} rtsp_user;

void launch_rtsp_server(void *video_engine,
                        unsigned int port,
                        char *watchVariable,
                        int auth,
                        rtsp_user *users, size_t users_size,
                        char *path[], size_t path_size);

#ifdef __cplusplus
}
#endif
     
#endif

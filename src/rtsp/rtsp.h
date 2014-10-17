#ifndef __RTSP_H__
#define __RTSP_H__

#include <base_app.h>

#define IPCAM_TYPE_RTSP (ipcam_rtsp_get_type())
#define IPCAM_RTSP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_TYPE_RTSP, IpcamRtsp))
#define IPCAM_RTSP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_TYPE_RTSP, IpcamRtspClass))
#define IPCAM_IS_RTSP(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_TYPE_RTSP))
#define IPCAM_IS_RTSP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_TYPE_RTSP))
#define IPCAM_RTSP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_TYPE_RTSP, IpcamRtspClass))

typedef struct _IpcamRtsp IpcamRtsp;
typedef struct _IpcamRtspClass IpcamRtspClass;

struct _IpcamRtsp
{
    GObject parent;
};

struct _IpcamRtspClass
{
    GObjectClass parent_class;
};

GType ipcam_rtsp_get_type(void);

#endif /* __RTSP_H__ */

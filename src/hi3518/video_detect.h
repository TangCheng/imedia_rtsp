#ifndef __VIDEO_DETECT_H__
#define __VIDEO_DETECT_H__

#include <glib.h>
#include <glib-object.h>
#include "stream_descriptor.h"

#define IPCAM_VIDEO_DETECT_TYPE (ipcam_video_detect_get_type())
#define IPCAM_VIDEO_DETECT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_VIDEO_DETECT_TYPE, IpcamVideoDetect))
#define IPCAM_VIDEO_DETECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_VIDEO_DETECT_TYPE, IpcamVideoDetectClass))
#define IPCAM_IS_VIDEO_DETECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_VIDEO_DETECT_TYPE))
#define IPCAM_IS_VIDEO_DETECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_VIDEO_DETECT_TYPE))
#define IPCAM_VIDEO_DETECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_VIDEO_DETECT_TYPE, IpcamVideoDetectClass))

typedef struct _IpcamVideoDetect IpcamVideoDetect;
typedef struct _IpcamVideoDetectClass IpcamVideoDetectClass;

struct _IpcamVideoDetect
{
    GObject parent;
};

struct _IpcamVideoDetectClass
{
    GObjectClass parent_class;
};

GType ipcam_video_detect_get_type(void);
gint32 ipcam_video_detect_start(IpcamVideoDetect *self, StreamDescriptor desc[]);
gint32 ipcam_video_detect_stop(IpcamVideoDetect *self);
void ipcam_video_detect_param_change(IpcamVideoDetect *self, StreamDescriptor desc[]);

#endif /* __VIDEO_DETECT_H__ */

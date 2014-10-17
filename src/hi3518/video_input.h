#ifndef __VIDEO_INPUT_H__
#define __VIDEO_INPUT_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_VIDEO_INPUT_TYPE (ipcam_video_input_get_type())
#define IPCAM_VIDEO_INPUT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_VIDEO_INPUT_TYPE, IpcamVideoInput))
#define IPCAM_VIDEO_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_VIDEO_INPUT_TYPE, IpcamVideoInputClass))
#define IPCAM_IS_VIDEO_INPUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_VIDEO_INPUT_TYPE))
#define IPCAM_IS_VIDEO_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_VIDEO_INPUT_TYPE))
#define IPCAM_VIDEO_INPUT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_VIDEO_INPUT_TYPE, IpcamVideoInputClass))

typedef struct _IpcamVideoInput IpcamVideoInput;
typedef struct _IpcamVideoInputClass IpcamVideoInputClass;

struct _IpcamVideoInput
{
    GObject parent;
};

struct _IpcamVideoInputClass
{
    GObjectClass parent_class;
};

GType ipcam_video_input_get_type(void);
gint32 ipcam_video_input_start(IpcamVideoInput *self);
gint32 ipcam_video_input_stop(IpcamVideoInput *self);

#endif /* __VIDEO_INPUT_H__ */

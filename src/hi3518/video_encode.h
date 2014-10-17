#ifndef __VIDEO_ENCODE_H__
#define __VIDEO_ENCODE_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_VIDEO_ENCODE_TYPE (ipcam_video_encode_get_type())
#define IPCAM_VIDEO_ENCODE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_VIDEO_ENCODE_TYPE, IpcamVideoEncode))
#define IPCAM_VIDEO_ENCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_VIDEO_ENCODE_TYPE, IpcamVideoEncodeClass))
#define IPCAM_IS_VIDEO_ENCODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_VIDEO_ENCODE_TYPE))
#define IPCAM_IS_VIDEO_ENCODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_VIDEO_ENCODE_TYPE))
#define IPCAM_VIDEO_ENCODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_VIDEO_ENCODE_TYPE, IpcamVideoEncodeClass))

typedef struct _IpcamVideoEncode IpcamVideoEncode;
typedef struct _IpcamVideoEncodeClass IpcamVideoEncodeClass;

struct _IpcamVideoEncode
{
    GObject parent;
};

struct _IpcamVideoEncodeClass
{
    GObjectClass parent_class;
};

GType ipcam_video_encode_get_type(void);
gint32 ipcam_video_encode_start(IpcamVideoEncode *self);
gint32 ipcam_video_encode_stop(IpcamVideoEncode *self);

#endif /* __VIDEO_ENCODE_H__ */

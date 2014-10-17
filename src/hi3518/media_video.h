#ifndef __MEDIA_VIDEO_H__
#define __MEDIA_VIDEO_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_MEDIA_VIDEO_TYPE (ipcam_media_video_get_type())
#define IPCAM_MEDIA_VIDEO(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_MEDIA_VIDEO_TYPE, IpcamMediaVideo))
#define IPCAM_MEDIA_VIDEO_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_MEDIA_VIDEO_TYPE, IpcamMediaVideoClass))
#define IPCAM_IS_MEDIA_VIDEO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_MEDIA_VIDEO_TYPE))
#define IPCAM_IS_MEDIA_VIDEO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_MEDIA_VIDEO_TYPE))
#define IPCAM_MEDIA_VIDEO_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_MEDIA_VIDEO_TYPE, IpcamMediaVideoClass))
#define IPCAM_MEDIA_VIDEO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), IPCAM_MEDIA_VIDEO_TYPE, IpcamMediaVideoPrivate))

typedef struct _IpcamMediaVideo IpcamMediaVideo;
typedef struct _IpcamMediaVideoClass IpcamMediaVideoClass;

struct _IpcamMediaVideo
{
    GObject parent;
};

struct _IpcamMediaVideoClass
{
    GObjectClass parent_class;
};

GType ipcam_media_video_get_type(void);

#endif /* __MEDIA_VIDEO_H__ */

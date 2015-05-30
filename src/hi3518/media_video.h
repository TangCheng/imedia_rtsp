#ifndef __MEDIA_VIDEO_H__
#define __MEDIA_VIDEO_H__

#include <glib.h>
#include <glib-object.h>
#include "video_detect.h"

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

gint32 ipcam_media_video_start_livestream(IpcamMediaVideo *self,
                                          StreamDescriptor desc[],
                                          OD_REGION_INFO od_reg_info[]);
gint32 ipcam_media_video_stop_livestream(IpcamMediaVideo *self);
void ipcam_media_video_param_change(IpcamMediaVideo *self,
                                    StreamDescriptor desc[],
                                    OD_REGION_INFO od_reg_info[]);

void ipcam_media_video_set_image_parameter(IpcamMediaVideo *self, IpcamMediaImageAttr *attr);
void ipcam_media_video_set_color2grey(IpcamMediaVideo *self, gboolean enabled);
void ipcam_media_video_set_antiflicker(IpcamMediaVideo *self, gboolean enable, guint8 freq);

#endif /* __MEDIA_VIDEO_H__ */

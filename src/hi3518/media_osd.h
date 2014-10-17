#ifndef __MEDIA_OSD_H__
#define __MEDIA_OSD_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_MEDIA_OSD_TYPE (ipcam_media_osd_get_type())
#define IPCAM_MEDIA_OSD(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_MEDIA_OSD_TYPE, IpcamMediaOsd))
#define IPCAM_MEDIA_OSD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_MEDIA_OSD_TYPE, IpcamMediaOsdClass))
#define IPCAM_IS_MEDIA_OSD(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_MEDIA_OSD_TYPE))
#define IPCAM_IS_MEDIA_OSD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_MEDIA_OSD_TYPE))
#define IPCAM_MEDIA_OSD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_MEDIA_OSD_TYPE, IpcamMediaOsdClass))
#define IPCAM_MEDIA_OSD_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), IPCAM_MEDIA_OSD_TYPE, IpcamMediaOsdPrivate))

typedef struct _IpcamMediaOsd IpcamMediaOsd;
typedef struct _IpcamMediaOsdClass IpcamMediaOsdClass;

struct _IpcamMediaOsd
{
    GObject parent;
};

struct _IpcamMediaOsdClass
{
    GObjectClass parent_class;
};

GType ipcam_media_osd_get_type(void);

#endif /* __MEDIA_OSD_H__ */

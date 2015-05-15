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

typedef enum _IPCAM_OSD_TYPE
{
    IPCAM_OSD_TYPE_DATETIME      = 0,
    IPCAM_OSD_TYPE_FRAMERATE     = 1,
    IPCAM_OSD_TYPE_BITRATE       = 2,
    IPCAM_OSD_TYPE_DEVICE_NAME   = 3,
    IPCAM_OSD_TYPE_COMMENT       = 4,
    IPCAM_OSD_TYPE_TRAIN_NUM     = 5,
    IPCAM_OSD_TYPE_CARRIAGE_NUM  = 6,
    IPCAM_OSD_TYPE_POSITION_NUM  = 7,
    IPCAM_OSD_TYPE_LAST
} IPCAM_OSD_TYPE;

typedef struct _Point
{
    guint32 x;
    guint32 y;
} Point;

typedef union _Color
{
    struct
    {
        guint32 blue:8;
        guint32 green:8;
        guint32 red:8;
        guint32 alpha:8;
    };
    guint32 value;
} Color;

typedef struct _IpcamOSDParameter
{
    gboolean is_show;
    Point position;
    guint font_size;
    Color color;
} IpcamOSDParameter;

struct _IpcamMediaOsd
{
    GObject parent;
};

struct _IpcamMediaOsdClass
{
    GObjectClass parent_class;
};

GType ipcam_media_osd_get_type(void);

gint32 ipcam_media_osd_start(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, IpcamOSDParameter *parameter);
gint32 ipcam_media_osd_show(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gboolean show);
gint32 ipcam_media_osd_set_pos(IpcamMediaOsd *self, IPCAM_OSD_TYPE type,  const Point pos);
gint32 ipcam_media_osd_set_fontsize(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const guint fsize);
gint32 ipcam_media_osd_set_color(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const Color color);
gint32 ipcam_media_osd_set_content(IpcamMediaOsd *self, IPCAM_OSD_TYPE type, const gchar *content);
gint32 ipcam_media_osd_invalidate(IpcamMediaOsd *self);
gint32 ipcam_media_osd_stop(IpcamMediaOsd *self);
void ipcam_media_osd_set_image_size(IpcamMediaOsd *self, guint32 width, guint32 height);

#endif /* __MEDIA_OSD_H__ */

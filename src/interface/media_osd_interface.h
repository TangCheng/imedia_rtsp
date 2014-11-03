#ifndef __MEDIA_OSD_INTERFACE_H__
#define __MEDIA_OSD_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_IOSD (ipcam_iosd_get_type())
#define IPCAM_IOSD(obj) (G_TYPE_CHECK_INSTANCE_CASE((obj), IPCAM_TYPE_IOSD, IpcamIOSD))
#define IPCAM_IS_IOSD(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_TYPE_IOSD))
#define IPCAM_IOSD_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), IPCAM_TYPE_IOSD, IpcamIOSDInterface))

typedef struct _IpcamIOSD IpcamIOSD;
typedef struct _IpcamIOSDInterface IpcamIOSDInterface;

typedef enum _IPCAM_OSD_TYPE
{
    IPCAM_OSD_TYPE_DATETIME      = 0,
    IPCAM_OSD_TYPE_FRAMERATE     = 1,
    IPCAM_OSD_TYPE_BITRATE       = 2,
    IPCAM_OSD_TYPE_DEVICE_NAME   = 3,
    IPCAM_OSD_TYPE_COMMENT       = 4,
    IPCAM_OSD_TYPE_TRAIN_NUM     = 5,
    IPCAM_OSD_TYPE_TRUNK_NUM     = 6,
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

struct _IpcamIOSDInterface
{
    GTypeInterface parent_iface;
    gint32 (*start)(IpcamIOSD *self, IPCAM_OSD_TYPE type, IpcamOSDParameter *parameter);
    gint32 (*show)(IpcamIOSD *self, IPCAM_OSD_TYPE type, const gboolean show);
    gint32 (*set_pos)(IpcamIOSD *self, IPCAM_OSD_TYPE type, const Point pos);
    gint32 (*set_fontsize)(IpcamIOSD *self, IPCAM_OSD_TYPE type, const guint fsize);
    gint32 (*set_color)(IpcamIOSD *self, IPCAM_OSD_TYPE type, const Color color);
    gint32 (*set_content)(IpcamIOSD *self, IPCAM_OSD_TYPE type, const gchar *content);
    gint32 (*invalidate)(IpcamIOSD *self);
    gint32 (*stop)(IpcamIOSD *self);
};

GType ipcam_iosd_get_type(void);
gint32 ipcam_iosd_start(IpcamIOSD *self, IPCAM_OSD_TYPE type, IpcamOSDParameter *parameter);
gint32 ipcam_iosd_show(IpcamIOSD *self, IPCAM_OSD_TYPE type, const gboolean show);
gint32 ipcam_iosd_set_pos(IpcamIOSD *self, IPCAM_OSD_TYPE type, const Point pos);
gint32 ipcam_iosd_set_fontsize(IpcamIOSD *self, IPCAM_OSD_TYPE type, const guint fsize);
gint32 ipcam_iosd_set_color(IpcamIOSD *self, IPCAM_OSD_TYPE type, const Color color);
gint32 ipcam_iosd_set_content(IpcamIOSD *self, IPCAM_OSD_TYPE type, const gchar *content);
gint32 ipcam_iosd_invalidate(IpcamIOSD *self);
gint32 ipcam_iosd_stop(IpcamIOSD *self);

G_END_DECLS

#endif /* __MEDIA_OSD_INTERFACE_H__  */

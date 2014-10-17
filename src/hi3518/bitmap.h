#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <glib.h>
#include <glib-object.h>
#include <hi_common.h>

#define IPCAM_BITMAP_TYPE (ipcam_bitmap_get_type())
#define IPCAM_BITMAP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_BITMAP_TYPE, IpcamBitmap))
#define IPCAM_BITMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_BITMAP_TYPE, IpcamBitmapClass))
#define IPCAM_IS_BITMAP(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_BITMAP_TYPE))
#define IPCAM_IS_BITMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_BITMAP_TYPE))
#define IPCAM_BITMAP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_BITMAP_TYPE, IpcamBitmapClass))

typedef struct _IpcamBitmap IpcamBitmap;
typedef struct _IpcamBitmapClass IpcamBitmapClass;

struct _IpcamBitmap
{
    GObject parent;
};

struct _IpcamBitmapClass
{
    GObjectClass parent_class;
};

GType ipcam_bitmap_get_type(void);
BITMAP_S *ipcam_bitmap_get_data(IpcamBitmap *self);
void ipcam_bitmap_clear(IpcamBitmap *self, RECT_S *area);
void ipcam_bitmap_bitblt(IpcamBitmap *self, BITMAP_S *src, POINT_S *pos);

#endif /* __BITMAP_H__ */

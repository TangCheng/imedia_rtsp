#ifndef __OSD_FONT_H__
#define __OSD_FONT_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_OSD_FONT_TYPE (ipcam_osd_font_get_type())
#define IPCAM_OSD_FONT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_OSD_FONT_TYPE, IpcamOsdFont))
#define IPCAM_OSD_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_OSD_FONT_TYPE, IpcamOsdFontClass))
#define IPCAM_IS_OSD_FONT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_OSD_FONT_TYPE))
#define IPCAM_IS_OSD_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_OSD_FONT_TYPE))
#define IPCAM_OSD_FONT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_OSD_FONT_TYPE, IpcamOsdFontClass))

typedef struct _IpcamOsdFont IpcamOsdFont;
typedef struct _IpcamOsdFontClass IpcamOsdFontClass;

struct _IpcamOsdFont
{
    GObject parent;
};

struct _IpcamOsdFontClass
{
    GObjectClass parent_class;
};

GType ipcam_osd_font_get_type(void);
IpcamOsdFont *ipcam_osd_font_new(void);
gboolean ipcam_osd_font_render_text(IpcamOsdFont *self,
                                    const gchar *string,
                                    void **data,
                                    guint *widht,
                                    guint *height);

#endif /* __OSD_FONT_H__ */

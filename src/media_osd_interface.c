#include "media_osd_interface.h"

G_DEFINE_INTERFACE(IpcamIOSD, ipcam_iosd, 0);

static void
ipcam_iosd_default_init(IpcamIOSDInterface *iface)
{

}

gint32 ipcam_iosd_start(IpcamIOSD *self, IPCAM_OSD_TYPE type, IpcamOSDParameter *parameter)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->start(self, type, parameter);
}

gint32 ipcam_iosd_show(IpcamIOSD *self, IPCAM_OSD_TYPE type, const gboolean show)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    g_return_val_if_fail((type < IPCAM_OSD_TYPE_LAST), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->show(self, type, show);
}

gint32 ipcam_iosd_set_pos(IpcamIOSD *self, IPCAM_OSD_TYPE type, const Point pos)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    g_return_val_if_fail((type < IPCAM_OSD_TYPE_LAST), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->set_pos(self, type, pos);
}

gint32 ipcam_iosd_set_fontsize(IpcamIOSD *self, IPCAM_OSD_TYPE type, const guint fsize)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    g_return_val_if_fail((type < IPCAM_OSD_TYPE_LAST), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->set_fontsize(self, type, fsize);
}

gint32 ipcam_iosd_set_color(IpcamIOSD *self, IPCAM_OSD_TYPE type, const Color color)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    g_return_val_if_fail((type < IPCAM_OSD_TYPE_LAST), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->set_color(self, type, color);
}

gint32 ipcam_iosd_set_content(IpcamIOSD *self, IPCAM_OSD_TYPE type, const gchar *content)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    g_return_val_if_fail((type < IPCAM_OSD_TYPE_LAST), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->set_content(self, type, content);
}

gint32 ipcam_iosd_invalidate(IpcamIOSD *self)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->invalidate(self);
}

gint32 ipcam_iosd_stop(IpcamIOSD *self)
{
    g_return_val_if_fail(IPCAM_IS_IOSD(self), FALSE);
    return IPCAM_IOSD_GET_INTERFACE(self)->stop(self);
}

#include "media_video_interface.h"

G_DEFINE_INTERFACE(IpcamIVideo, ipcam_ivideo, 0);

static void
ipcam_ivideo_default_init(IpcamIVideoInterface *iface)
{

}

gint32 ipcam_ivideo_start(IpcamIVideo *self)
{
    g_return_val_if_fail(IPCAM_IS_IVIDEO(self), FALSE);
    return IPCAM_IVIDEO_GET_INTERFACE(self)->start(self);
}

gint32 ipcam_ivideo_stop(IpcamIVideo *self)
{
    g_return_val_if_fail(IPCAM_IS_IVIDEO(self), FALSE);
    return IPCAM_IVIDEO_GET_INTERFACE(self)->stop(self);
}

#include "media_video_interface.h"

G_DEFINE_INTERFACE(IpcamIVideo, ipcam_ivideo, 0);

static void
ipcam_ivideo_default_init(IpcamIVideoInterface *iface)
{

}

gint32 ipcam_ivideo_start(IpcamIVideo *self, StreamDescriptor desc[])
{
    g_return_val_if_fail(IPCAM_IS_IVIDEO(self), FALSE);
    return IPCAM_IVIDEO_GET_INTERFACE(self)->start(self, desc);
}

gint32 ipcam_ivideo_stop(IpcamIVideo *self)
{
    g_return_val_if_fail(IPCAM_IS_IVIDEO(self), FALSE);
    return IPCAM_IVIDEO_GET_INTERFACE(self)->stop(self);
}

void ipcam_ivideo_param_change(IpcamIVideo *self, StreamDescriptor desc[])
{
    g_return_if_fail(IPCAM_IS_IVIDEO(self));
    IPCAM_IVIDEO_GET_INTERFACE(self)->param_change(self, desc);
}

gpointer ipcam_ivideo_get_video_data(IpcamIVideo *self, StreamChannel chn)
{
    g_return_val_if_fail(IPCAM_IS_IVIDEO(self), NULL);
    return IPCAM_IVIDEO_GET_INTERFACE(self)->get_video_data(self, chn);
}

void ipcam_ivideo_release_video_data(IpcamIVideo *self, StreamChannel chn, gpointer data)
{
    g_return_if_fail(IPCAM_IS_IVIDEO(self));
    IPCAM_IVIDEO_GET_INTERFACE(self)->release_video_data(self, chn, data);
}

gboolean ipcam_ivideo_has_video_data(IpcamIVideo *self, StreamChannel chn)
{
    g_return_val_if_fail(IPCAM_IS_IVIDEO(self), FALSE);
    return IPCAM_IVIDEO_GET_INTERFACE(self)->has_video_data(self, chn);
}

void ipcam_ivideo_register_rtsp_source(IpcamIVideo *self, StreamChannel chn, void *source)
{
    g_return_if_fail(IPCAM_IS_IVIDEO(self));
    IPCAM_IVIDEO_GET_INTERFACE(self)->register_rtsp_source(self, chn, source);
}

void ipcam_ivideo_unregister_rtsp_source(IpcamIVideo *self, StreamChannel chn, void *source)
{
    g_return_if_fail(IPCAM_IS_IVIDEO(self));
    IPCAM_IVIDEO_GET_INTERFACE(self)->unregister_rtsp_source(self, chn, source);
}

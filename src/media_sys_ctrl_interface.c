#include "media_sys_ctrl_interface.h"

G_DEFINE_INTERFACE(IpcamIMediaSysCtrl, ipcam_imedia_sys_ctrl, 0);

static void
ipcam_imedia_sys_ctrl_default_init(IpcamIMediaSysCtrlInterface *iface)
{

}

void
ipcam_imedia_sys_ctrl_init(IpcamIMediaSysCtrl *self)
{
    g_return_if_fail(IPCAM_IS_IMEDIA_SYS_CTRL(self));
    IPCAM_IMEDIA_SYS_CTRL_GET_INTERFACE(self)->init_media_system(self);
}

void
ipcam_imedia_sys_ctrl_uninit(IpcamIMediaSysCtrl *self)
{
    g_return_if_fail(IPCAM_IS_IMEDIA_SYS_CTRL(self));
    IPCAM_IMEDIA_SYS_CTRL_GET_INTERFACE(self)->uninit_media_system(self);
}

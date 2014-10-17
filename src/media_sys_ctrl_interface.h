#ifndef __MEDIA_SYS_CTRL_INTERFACE_H__
#define __MEDIA_SYS_CTRL_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_IMEDIA_SYS_CTRL (ipcam_imedia_sys_ctrl_get_type())
#define IPCAM_IMEDIA_SYS_CTRL(obj) (G_TYPE_CHECK_INSTANCE_CASE((obj), IPCAM_TYPE_IMEDIA_SYS_CTRL, IpcamIMediaSysCtrl))
#define IPCAM_IS_IMEDIA_SYS_CTRL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_TYPE_IMEDIA_SYS_CTRL))
#define IPCAM_IMEDIA_SYS_CTRL_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), IPCAM_TYPE_IMEDIA_SYS_CTRL, IpcamIMediaSysCtrlInterface))

typedef struct _IpcamIMediaSysCtrl IpcamIMediaSysCtrl;
typedef struct _IpcamIMediaSysCtrlInterface IpcamIMediaSysCtrlInterface;

struct _IpcamIMediaSysCtrlInterface
{
    GTypeInterface parent_iface;
    void (*init_media_system)(IpcamIMediaSysCtrl *self);
    void (*uninit_media_system)(IpcamIMediaSysCtrl *self);
};

GType ipcam_imedia_sys_ctrl_get_type(void);

void ipcam_imedia_sys_ctrl_init(IpcamIMediaSysCtrl *self);
void ipcam_imedia_sys_ctrl_uninit(IpcamIMediaSysCtrl *self);

G_END_DECLS

#endif /* __MEDIA_SYS_CTRL_INTERFACE_H__  */

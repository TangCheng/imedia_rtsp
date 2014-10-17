#ifndef __MEDIA_SYS_CTRL_H__
#define __MEDIA_SYS_CTRL_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_MEDIA_SYS_CTRL_TYPE (ipcam_media_sys_ctrl_get_type())
#define IPCAM_MEDIA_SYS_CTRL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_MEDIA_SYS_CTRL_TYPE, IpcamMediaSysCtrl))
#define IPCAM_MEDIA_SYS_CTRL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_MEDIA_SYS_CTRL_TYPE, IpcamMediaSysCtrlClass))
#define IPCAM_IS_MEDIA_SYS_CTRL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_MEDIA_SYS_CTRL_TYPE))
#define IPCAM_IS_MEDIA_SYS_CTRL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_MEDIA_SYS_CTRL_TYPE))
#define IPCAM_MEDIA_SYS_CTRL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_MEDIA_SYS_CTRL_TYPE, IpcamMediaSysCtrlClass))

typedef struct _IpcamMediaSysCtrl IpcamMediaSysCtrl;
typedef struct _IpcamMediaSysCtrlClass IpcamMediaSysCtrlClass;

struct _IpcamMediaSysCtrl
{
    GObject parent;
};

struct _IpcamMediaSysCtrlClass
{
    GObjectClass parent_class;
};

GType ipcam_media_sys_ctrl_get_type(void);

#endif /* __MEDIA_SYS_CTRL_H__ */

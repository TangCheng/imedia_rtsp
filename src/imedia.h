#ifndef __IMEDIA_H__
#define __IMEDIA_H__

#include <base_app.h>

#define IPCAM_IMEDIA_TYPE (ipcam_imedia_get_type())
#define IPCAM_IMEDIA(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_IMEDIA_TYPE, IpcamIMedia))
#define IPCAM_IMEDIA_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_IMEDIA_TYPE, IpcamIMediaClass))
#define IPCAM_IS_IMEDIA(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_IMEDIA_TYPE))
#define IPCAM_IS_IMEDIA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_IMEDIA_TYPE))
#define IPCAM_IMEDIA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_IMEDIA_TYPE, IpcamIMediaClass))

typedef struct _IpcamIMedia IpcamIMedia;
typedef struct _IpcamIMediaClass IpcamIMediaClass;

struct _IpcamIMedia
{
    IpcamBaseApp parent;
};

struct _IpcamIMediaClass
{
    IpcamBaseAppClass parent_class;
};

GType ipcam_imedia_get_type(void);

#endif /* __IMEDIA_H__ */

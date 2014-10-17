#ifndef __ISP_H__
#define __ISP_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_ISP_TYPE (ipcam_isp_get_type())
#define IPCAM_ISP(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_ISP_TYPE, IpcamIsp))
#define IPCAM_ISP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_ISP_TYPE, IpcamIspClass))
#define IPCAM_IS_ISP(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_ISP_TYPE))
#define IPCAM_IS_ISP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_ISP_TYPE))
#define IPCAM_ISP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_ISP_TYPE, IpcamIspClass))

typedef struct _IpcamIsp IpcamIsp;
typedef struct _IpcamIspClass IpcamIspClass;

struct _IpcamIsp
{
    GObject parent;
};

struct _IpcamIspClass
{
    GObjectClass parent_class;
};

GType ipcam_isp_get_type(void);
gint32 ipcam_isp_start(IpcamIsp *self);
void ipcam_isp_stop(IpcamIsp *self);

#endif /* __ISP_H__ */

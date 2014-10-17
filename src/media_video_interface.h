#ifndef __MEDIA_VIDEO_INTERFACE_H__
#define __MEDIA_VIDEO_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_IVIDEO (ipcam_ivideo_get_type())
#define IPCAM_IVIDEO(obj) (G_TYPE_CHECK_INSTANCE_CASE((obj), IPCAM_TYPE_IVIDEO, IpcamIVideo))
#define IPCAM_IS_IVIDEO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_TYPE_IVIDEO))
#define IPCAM_IVIDEO_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE((inst), IPCAM_TYPE_IVIDEO, IpcamIVideoInterface))

typedef struct _IpcamIVideo IpcamIVideo;
typedef struct _IpcamIVideoInterface IpcamIVideoInterface;

struct _IpcamIVideoInterface
{
    GTypeInterface parent_iface;
    gint32 (*start)(IpcamIVideo *self);
    gint32 (*stop)(IpcamIVideo *self);
};

GType ipcam_ivideo_get_type(void);
gint32 ipcam_ivideo_start(IpcamIVideo *self);
gint32 ipcam_ivideo_stop(IpcamIVideo *self);

G_END_DECLS

#endif /* __MEDIA_VIDEO_INTERFACE_H__  */

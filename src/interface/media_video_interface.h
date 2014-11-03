#ifndef __MEDIA_VIDEO_INTERFACE_H__
#define __MEDIA_VIDEO_INTERFACE_H__

#include <glib.h>
#include <glib-object.h>
#include "stream_descriptor.h"

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
    gint32 (*start)(IpcamIVideo *self, StreamDescriptor desc[]);
    gint32 (*stop)(IpcamIVideo *self);
    void (*param_change)(IpcamIVideo *self, StreamDescriptor desc[]);
    gpointer (*get_video_data)(IpcamIVideo *self);
    void (*release_video_data)(IpcamIVideo *self, gpointer data);
    gboolean (*has_video_data)(IpcamIVideo *self);
    void (*register_rtsp_source)(IpcamIVideo *self, void *source);
    void (*unregister_rtsp_source)(IpcamIVideo *self, void *source);
};

GType ipcam_ivideo_get_type(void);
gint32 ipcam_ivideo_start(IpcamIVideo *self, StreamDescriptor desc[]);
gint32 ipcam_ivideo_stop(IpcamIVideo *self);
void ipcam_ivideo_paran_change(IpcamIVideo *self, StreamDescriptor desc[]);
gpointer ipcam_ivideo_get_video_data(IpcamIVideo *self);
void ipcam_ivideo_release_video_data(IpcamIVideo *self, gpointer data);
gboolean ipcam_ivideo_has_video_data(IpcamIVideo *self);
void ipcam_ivideo_register_rtsp_source(IpcamIVideo *self, void *source);
void ipcam_ivideo_unregister_rtsp_source(IpcamIVideo *self, void *source);

G_END_DECLS

#endif /* __MEDIA_VIDEO_INTERFACE_H__  */

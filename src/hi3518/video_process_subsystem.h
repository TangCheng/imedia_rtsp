#ifndef __VIDEO_PROCESS_SUBSYSTEM_H__
#define __VIDEO_PROCESS_SUBSYSTEM_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE (ipcam_video_process_subsystem_get_type())
#define IPCAM_VIDEO_PROCESS_SUBSYSTEM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE, IpcamVideoProcessSubsystem))
#define IPCAM_VIDEO_PROCESS_SUBSYSTEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE, IpcamVideoProcessSubsystemClass))
#define IPCAM_IS_VIDEO_PROCESS_SUBSYSTEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE))
#define IPCAM_IS_VIDEO_PROCESS_SUBSYSTEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE))
#define IPCAM_VIDEO_PROCESS_SUBSYSTEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_VIDEO_PROCESS_SUBSYSTEM_TYPE, IpcamVideoProcessSubsystemClass))

typedef struct _IpcamVideoProcessSubsystem IpcamVideoProcessSubsystem;
typedef struct _IpcamVideoProcessSubsystemClass IpcamVideoProcessSubsystemClass;

struct _IpcamVideoProcessSubsystem
{
    GObject parent;
};

struct _IpcamVideoProcessSubsystemClass
{
    GObjectClass parent_class;
};

GType ipcam_video_process_subsystem_get_type(void);
gint32 ipcam_video_process_subsystem_start(IpcamVideoProcessSubsystem *self);
gint32 ipcam_video_process_subsystem_stop(IpcamVideoProcessSubsystem *self);

#endif /* __VIDEO_PROCESS_SUBSYSTEM_H__ */

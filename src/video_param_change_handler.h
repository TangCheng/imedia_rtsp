#ifndef __VIDEO_PARAM_CHANGE_HANDLER_H__
#define __VIDEO_PARAM_CHANGE_HANDLER_H__

#include "event_handler.h"

#define IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE (ipcam_video_param_change_handler_get_type())
#define IPCAM_VIDEO_PARAM_CHANGE_HANDLER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE, IpcamVideoParamChangeHandler))
#define IPCAM_VIDEO_PARAM_CHANGE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE, IpcamVideoParamChangeHandlerClass))
#define IPCAM_IS_VIDEO_PARAM_CHANGE_HANDLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE))
#define IPCAM_IS_VIDEO_PARAM_CHANGE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE))
#define IPCAM_VIDEO_PARAM_CHANGE_HANDLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_VIDEO_PARAM_CHANGE_HANDLER_TYPE, IpcamVideoParamChangeHandlerClass))

typedef struct _IpcamVideoParamChangeHandler IpcamVideoParamChangeHandler;
typedef struct _IpcamVideoParamChangeHandlerClass IpcamVideoParamChangeHandlerClass;

struct _IpcamVideoParamChangeHandler
{
    IpcamEventHandler parent;
};

struct _IpcamVideoParamChangeHandlerClass
{
    IpcamEventHandlerClass parent_class;
};

GType ipcam_video_param_change_handler_get_type(void);

#endif /* __VIDEO_PARAM_CHANGE_HANDLER_H__ */

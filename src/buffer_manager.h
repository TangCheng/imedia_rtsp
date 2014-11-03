#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include <glib.h>
#include <glib-object.h>

#define IPCAM_BUFFER_MANAGER_TYPE (ipcam_buffer_manager_get_type())
#define IPCAM_BUFFER_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_BUFFER_MANAGER_TYPE, IpcamBufferManager))
#define IPCAM_BUFFER_MANAGER_CLASS(klass) (G_TYEP_CHECK_CLASS_CAST((klass), IPCAM_BUFFER_MANAGER_TYPE, IpcamBufferManagerClass))
#define IPCAM_IS_BUFFER_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_BUFFER_MANAGER_TYPE))
#define IPCAM_IS_BUFFER_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_BUFFER_MANAGER_TYPE))
#define IPCAM_BUFFER_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_BUFFER_MANAGER_TYPE, IpcamBufferManagerClass))

typedef struct _IpcamBufferManager IpcamBufferManager;
typedef struct _IpcamBufferManagerClass IpcamBufferManagerClass;

struct _IpcamBufferManager
{
    GObject parent;
};

struct _IpcamBufferManagerClass
{
    GObjectClass parent_class;
};

GType ipcam_buffer_manager_get_type(void);
gpointer ipcam_buffer_manager_get_write_data(IpcamBufferManager *buffer_manager);
void ipcam_buffer_manager_release_write_data(IpcamBufferManager *buffer_manager, gpointer data);
gpointer ipcam_buffer_manager_get_read_data(IpcamBufferManager *buffer_manager);
void ipcam_buffer_manager_release_read_data(IpcamBufferManager *buffer_manager, gpointer data);
gboolean ipcam_buffer_manager_has_data(IpcamBufferManager *buffer_manager);

#endif /* __BUFFER_MANAGER_H__ */

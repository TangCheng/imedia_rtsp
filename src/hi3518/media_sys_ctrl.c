#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_vb.h>
#include <mpi_sys.h>
#include <mpi_vb.h>
#include <memory.h>
#include "media_sys_ctrl.h"
#include "stream_descriptor.h"

G_DEFINE_TYPE(IpcamMediaSysCtrl, ipcam_media_sys_ctrl, G_TYPE_OBJECT);

#define SYS_ALIGN_WIDTH      16

static void ipcam_media_sys_ctrl_finalize(GObject *object)
{
    ipcam_media_sys_ctrl_uninit_media_system(IPCAM_MEDIA_SYS_CTRL(object));
    G_OBJECT_CLASS(ipcam_media_sys_ctrl_parent_class)->finalize(object);
}
static void ipcam_media_sys_ctrl_init(IpcamMediaSysCtrl *self)
{
}
static void ipcam_media_sys_ctrl_class_init(IpcamMediaSysCtrlClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_media_sys_ctrl_finalize;
}

void ipcam_media_sys_ctrl_init_media_system(IpcamMediaSysCtrl *self)
{
    VB_CONF_S stVbConf;
    MPP_SYS_CONF_S stSysConf = {0};
    HI_S32 s32Ret = HI_FAILURE;

    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = VB_MAX_POOLS;
    stVbConf.astCommPool[0].u32BlkSize = (CEILING_2_POWER(IMAGE_MAX_WIDTH, SYS_ALIGN_WIDTH) * \
                                          CEILING_2_POWER(IMAGE_MAX_HEIGHT, SYS_ALIGN_WIDTH) * \
                                          2);
    stVbConf.astCommPool[0].u32BlkCnt = 12;
    memset(stVbConf.astCommPool[0].acMmzName, 0, sizeof(stVbConf.astCommPool[0].acMmzName));

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VB_SetConf failed with %#x!\n", s32Ret);
        return;
    }

    s32Ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_VB_Init failed with %#x!\n", s32Ret);
        return;
    }

    stSysConf.u32AlignWidth = SYS_ALIGN_WIDTH;
    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_SYS_SetConf failed with %#x!\n", s32Ret);
        return;
    }

    s32Ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32Ret)
    {
        g_critical("HI_MPI_SYS_Init failed with %#x!\n", s32Ret);
        return;
    }
}

void ipcam_media_sys_ctrl_uninit_media_system(IpcamMediaSysCtrl *self)
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
}

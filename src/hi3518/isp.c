#include <unistd.h>
#include <hi_defines.h>
#include <hi_comm_sys.h>
#include <hi_comm_3a.h>
#include <hi_ae_comm.h>
#include <hi_awb_comm.h>
#include <hi_af_comm.h>
#include <mpi_sys.h>
#include <mpi_isp.h>
#include <mpi_ae.h>
#include <mpi_awb.h>
#include <mpi_af.h>
#include <hi_sns_ctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include "isp.h"

guint32 sensor_image_width;
guint32 sensor_image_height;
guint32 sensor_frame_rate;

typedef struct _IpcamIspPrivate
{
    gchar *sensor_type;
    //guint32 image_width;
    //guint32 image_height;
    //guint32 fps;
    void *sensor_lib_handle;
    gint32 (*sensor_register_callback)();
    gint32 (*sensor_set_image_mode)(ISP_CMOS_SENSOR_IMAGE_MODE *);
    gint32 (*sensor_unregister_callback)(void);
    pthread_t IspPid;
} IpcamIspPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamIsp, ipcam_isp, G_TYPE_OBJECT)

static void ipcam_isp_init(IpcamIsp *self)
{
	IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);
    priv->sensor_type = NULL;
    sensor_image_width = 1920;
    sensor_image_height = 1080;
    sensor_frame_rate = 30;
    priv->sensor_lib_handle = NULL;
    priv->sensor_register_callback = NULL;
}
static void ipcam_isp_class_init(IpcamIspClass *klass)
{
}
static gint32 ipcam_isp_load_sensor_lib(IpcamIsp *self)
{
    IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);
    gchar *error;
    priv->sensor_type = getenv("SENSOR_TYPE");

    if (NULL == priv->sensor_type)
    {
        g_critical("%s: sensor type undefined!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    if (g_str_equal(priv->sensor_type, "AR0130"))
    {
        priv->sensor_lib_handle = dlopen("/usr/lib/libsns_ar0130_720p.so", RTLD_LAZY);
    }
    else if (g_str_equal(priv->sensor_type, "AR0331"))
    {
        priv->sensor_lib_handle = dlopen("/usr/lib/libsns_ar0331_1080p.so", RTLD_LAZY);
    }
    else if (g_str_equal(priv->sensor_type, "NT99141"))
    {
        priv->sensor_lib_handle = dlopen("/usr/lib/libsns_nt99141.so", RTLD_LAZY);
    }
    else if (g_str_equal(priv->sensor_type, "IMX222"))
    {
        priv->sensor_lib_handle = dlopen("/usr/lib/libsns_imx122_uxga.so", RTLD_LAZY);
    }
    else
    {
        g_warning("Unknown sensor type %s\n!", priv->sensor_type);
    }
    if (priv->sensor_lib_handle)
    {
        priv->sensor_register_callback = dlsym(priv->sensor_lib_handle, "sensor_register_callback");
        error = dlerror();
        if (NULL != error)
        {
            g_critical("%s: get sensor_register_callback failed with %s!\n", __FUNCTION__, error);
            return HI_FAILURE;
        }

        priv->sensor_unregister_callback = dlsym(priv->sensor_lib_handle, "sensor_unregister_callback");
        error = dlerror();
        if (NULL != error)
        {
            g_warning("%s: get sensor_unregister_callback failed with %s!\n", __FUNCTION__, error);
        }

        priv->sensor_set_image_mode = dlsym(priv->sensor_lib_handle, "sensor_set_image_mode");
        error = dlerror();
        if (NULL != error)
        {
            g_warning("%s: get sensor_set_image_size failed with %s!\n", __FUNCTION__, error);
        }
    }
    return priv->sensor_lib_handle ? HI_SUCCESS : HI_FAILURE;
}

static void ipcam_isp_init_image_attr(IpcamIsp *self, ISP_IMAGE_ATTR_S *pstImageAttr)
{
    IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);

    if (g_str_equal(priv->sensor_type, "AR0130") ||
        g_str_equal(priv->sensor_type, "NT99141"))
    {
        pstImageAttr->enBayer      = BAYER_GRBG;
        pstImageAttr->u16FrameRate = 30;
        pstImageAttr->u16Width     = 1280;
        pstImageAttr->u16Height    = 720;
    }
    else if (g_str_equal(priv->sensor_type, "AR0331"))
    {
        pstImageAttr->enBayer      = BAYER_GRBG;
        pstImageAttr->u16FrameRate = 30;
        pstImageAttr->u16Width     = 1920;
        pstImageAttr->u16Height    = 1080;
    }
    else if (g_str_equal(priv->sensor_type, "IMX222"))
    {
        pstImageAttr->enBayer      = BAYER_RGGB;
        if (sensor_image_height == 1200)
        {
            pstImageAttr->u16FrameRate = 30;
            pstImageAttr->u16Width     = 1920;
            pstImageAttr->u16Height    = 1200;
        }
        else
        {
            pstImageAttr->u16FrameRate = 30;
            pstImageAttr->u16Width     = 1920;
            pstImageAttr->u16Height    = 1080;
        }
    }
    else
    {
        g_warning("Unknown sensor type %s\n!", priv->sensor_type);
        g_warn_if_reached();
    }
}

static void ipcam_isp_init_input_timing(IpcamIsp *self, ISP_INPUT_TIMING_S *stInputTiming)
{
    IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);

    stInputTiming->enWndMode = ISP_WIND_NONE;
    if (g_str_equal(priv->sensor_type, "IMX222"))
    {
        if (sensor_image_height == 1200)
        {
            stInputTiming->enWndMode = ISP_WIND_ALL;
            stInputTiming->u16HorWndStart = 138;
            stInputTiming->u16HorWndLength = 1920;
            stInputTiming->u16VerWndStart = 22;
            stInputTiming->u16VerWndLength = 1200;
        }
        else
        {
            stInputTiming->enWndMode = ISP_WIND_ALL;
            stInputTiming->u16HorWndStart = 200;
            stInputTiming->u16HorWndLength = 1920;
            stInputTiming->u16VerWndStart = 12;
            stInputTiming->u16VerWndLength = 1080;
        }
    }
}

static void physical_address_writel(unsigned long phy_addr, HI_U32 val)
{
    volatile HI_VOID *base;

    base = HI_MPI_SYS_Mmap(0x20030000, 0x1000);
    if (base) {
        *(HI_U32*)(base + 0x30) = val;
        HI_MPI_SYS_Munmap((HI_VOID*)base, 0x1000);
    }
}

static void ipcam_isp_set_pixel_clock(IpcamIsp *self)
{
    /* Adjust the clock */
    if (sensor_image_height == 1200) {
        /* 54M */
        physical_address_writel(0x20030030, 0x03);
    }
    else {
        /* 37.125 */
        physical_address_writel(0x20030030, 0x06);
    }
    /* delay 200ms for clock stable */
    usleep(200000);
}

#define SENSOR_MODE_AUTO	1

static gboolean ipcam_isp_check_video_resolution(IpcamIsp *self, StreamDescriptor desc[])
{
    guint32 image_width;
    guint32 image_height;
    guint32 fps;

	char *sensor_mode = getenv("SENSOR_MODE");

	if (sensor_mode && (strncmp(sensor_mode, "1920x1200", 9) == 0)) {
		/* 1920x1200@20 */
		image_width = 1920;
		image_height = 1200;
		fps = 20;
	}
	else if (sensor_mode && (strncmp(sensor_mode, "1920x1080", 9) == 0)) {
		/* 1920x1080@30 */
		image_width = 1920;
		image_height = 1080;
		fps = 30;
	}
	else {
		/* Default: auto select sensor mode */
		const gchar *resolution = desc[MASTER_CHN].v_desc.resolution;
		if (g_str_equal(resolution, "UXGA") ||
			g_str_equal(resolution, "960H") ||
			desc[MASTER_CHN].v_desc.frame_rate <= 20)
		{
			image_width = 1920;
			image_height = 1200;
			fps = 20;
		}
		else
		{
			image_width = 1920;
			image_height = 1080;
			fps = 30;
		}
	}

    if (sensor_image_height != image_height) {
        sensor_image_width = image_width;
        sensor_image_height = image_height;
        sensor_frame_rate = fps;

        return TRUE;
    }

    return TRUE;
}

static void ipcam_isp_set_image_mode(IpcamIsp *self)
{
    IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);

    if (priv->sensor_set_image_mode)
    {
        ISP_CMOS_SENSOR_IMAGE_MODE stSensorImageMode = {
            .u16Width = sensor_image_width,
            .u16Height = sensor_image_height,
            .u16Fps = sensor_frame_rate
        };
        (*priv->sensor_set_image_mode)(&stSensorImageMode);
    }
}

void *isp_thread_routine(void *arg)
{
    prctl(PR_SET_NAME, (unsigned long)"ISP");

    return (void*)HI_MPI_ISP_Run();
}

gint32 ipcam_isp_start(IpcamIsp *self, StreamDescriptor desc[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);

    g_return_val_if_fail(IPCAM_IS_ISP(self), HI_FAILURE);
    g_return_val_if_fail(desc != NULL, HI_FAILURE);

    ipcam_isp_check_video_resolution(self, desc);

    ipcam_isp_set_pixel_clock(self);

    /******************************************
     step 1: configure sensor.
     note: you can jump over this step, if you do not use Hi3518 interal isp. 
    ******************************************/
    s32Ret = ipcam_isp_load_sensor_lib(self);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: ipcam_isp_load_sensor_lib failed!\n", __FUNCTION__);
        return s32Ret;
    }

    if (priv->sensor_register_callback)
    {   
        s32Ret = (*priv->sensor_register_callback)();
        if (s32Ret != HI_SUCCESS)
        {
            g_critical("%s: sensor_register_callback failed with %#x!\n", __FUNCTION__, s32Ret);
            return s32Ret;
        }
    }

	ISP_IMAGE_ATTR_S stImageAttr;
    ISP_INPUT_TIMING_S stInputTiming;

    ALG_LIB_S stLib;

    /* 1. register ae lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AE_LIB_NAME);
    s32Ret = HI_MPI_AE_Register(&stLib);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_AE_Register failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 2. register awb lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
    s32Ret = HI_MPI_AWB_Register(&stLib);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_AWB_Register failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 3. register af lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AF_LIB_NAME);
    s32Ret = HI_MPI_AF_Register(&stLib);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_AF_Register failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    /* 4. isp init */
    s32Ret = HI_MPI_ISP_Init();
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_ISP_Init failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    ipcam_isp_set_image_mode(self);

    /* 5. isp set image attributes */
    /* note : different sensor, different ISP_IMAGE_ATTR_S define.
              if the sensor you used is different, you can change
              ISP_IMAGE_ATTR_S definition */
    ipcam_isp_init_image_attr(self, &stImageAttr);
    s32Ret = HI_MPI_ISP_SetImageAttr(&stImageAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_ISP_SetImageAttr failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    ipcam_isp_init_input_timing(self, &stInputTiming);
    s32Ret = HI_MPI_ISP_SetInputTiming(&stInputTiming);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_ISP_SetInputTiming failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

#if 0
    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = 50;
    pthread_attr_setschedparam(&attr, &param);
    if (0 != pthread_create(&priv->IspPid, &attr, (void* (*)(void*))HI_MPI_ISP_Run, NULL))
#endif
    if (0 != pthread_create(&priv->IspPid, NULL, (void* (*)(void*))isp_thread_routine, NULL))
    {
        g_critical("%s: create isp running thread failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

void ipcam_isp_stop(IpcamIsp *self)
{
    IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self);

    g_return_if_fail(IPCAM_IS_ISP(self));

    HI_MPI_ISP_Exit();
    pthread_join(priv->IspPid, 0);

    if (priv->sensor_lib_handle)
    {
        if (priv->sensor_unregister_callback)
            (*priv->sensor_unregister_callback)();

        dlclose(priv->sensor_lib_handle);
        priv->sensor_lib_handle = NULL;
    }
    priv->sensor_type = NULL;
}

void ipcam_isp_param_change(IpcamIsp *self, StreamDescriptor desc[])
{
    /* IpcamIspPrivate *priv = ipcam_isp_get_instance_private(self); */
	ISP_IMAGE_ATTR_S stImageAttr;
    ISP_INPUT_TIMING_S stInputTiming;
    HI_S32 s32Ret = HI_SUCCESS;

    g_return_if_fail(IPCAM_IS_ISP(self));

    if (!ipcam_isp_check_video_resolution(self, desc))
        return;

    ipcam_isp_set_pixel_clock(self);

    ipcam_isp_set_image_mode(self);

    usleep(200000);

    ipcam_isp_init_image_attr(self, &stImageAttr);
    s32Ret = HI_MPI_ISP_SetImageAttr(&stImageAttr);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_ISP_SetImageAttr failed with %#x!\n", __FUNCTION__, s32Ret);
        return;
    }

    ipcam_isp_init_input_timing(self, &stInputTiming);
    s32Ret = HI_MPI_ISP_SetInputTiming(&stInputTiming);
    if (s32Ret != HI_SUCCESS)
    {
        g_critical("%s: HI_MPI_ISP_SetInputTiming failed with %#x!\n", __FUNCTION__, s32Ret);
        return;
    }
}

void ipcam_isp_set_antiflicker(IpcamIsp *isp, HI_BOOL enable, HI_U8 freq)
{
	HI_S32 s32Ret;
	ISP_ANTIFLICKER_S stAntiFlicker;

	stAntiFlicker.bEnable = enable;
	stAntiFlicker.u8Frequency = freq;
	stAntiFlicker.enMode = ISP_ANTIFLICKER_MODE_0;

	s32Ret = HI_MPI_ISP_SetAntiFlickerAttr(&stAntiFlicker);
    if (s32Ret != HI_SUCCESS) {
        g_critical("%s: HI_MPI_ISP_SetAntiFlickerAttr failed with %#x!\n",
				   __FUNCTION__, s32Ret);
    }
}

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hi_defines.h>
#include <hi_comm_aio.h>
#include <hi_comm_aenc.h>
#include <mpi_ai.h>
#include <mpi_aenc.h>
#include <acodec.h>
#include <memory.h>

#include "media_audio.h"

#define ACODEC_FILE		"/dev/acodec"

#define AUDIO_PTNUMPERFRM   320
#define AUDIO_ADPCM_TYPE    ADPCM_TYPE_DVI4 // ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4
#define G726_BPS            MEDIA_G726_40K  // MEDIA_G726_16K, MEDIA_G726_24K ...

struct IpcamMediaAudio
{
	guint   sample_rate;
	guint   bitrate;
};

IpcamMediaAudio *ipcam_media_audio_new(void)
{
	IpcamMediaAudio *audio = g_malloc0(sizeof(IpcamMediaAudio));
	audio->sample_rate = AUDIO_SAMPLE_RATE_8000;
	return audio;
}

void ipcam_media_audio_free(IpcamMediaAudio *audio)
{
	g_free(audio);
}

gint32 ipcam_media_audio_start(IpcamMediaAudio *audio, StreamDescriptor *desc)
{
	HI_S32 s32Ret;

	// Initialize Audio CODEC
	int fd = open(ACODEC_FILE, O_RDWR);
	if (fd < 0) {
		g_critical("Cannot open acodec %s\n", ACODEC_FILE);
		return HI_FAILURE;
	}

	if (ioctl(fd, ACODEC_SOFT_RESET_CTRL)) {
		g_critical("Failed to reset audio codec");
		close(fd);
		return HI_FAILURE;
	}

	unsigned int i2s_fs_sel = 0;
	switch(audio->sample_rate) {
    case AUDIO_SAMPLE_RATE_8000:
    case AUDIO_SAMPLE_RATE_11025:
    case AUDIO_SAMPLE_RATE_12000:
        i2s_fs_sel = 0x18;
        break;
    case AUDIO_SAMPLE_RATE_16000:
    case AUDIO_SAMPLE_RATE_22050:
    case AUDIO_SAMPLE_RATE_24000:
        i2s_fs_sel = 0x19;
        break;
    case AUDIO_SAMPLE_RATE_32000:
    case AUDIO_SAMPLE_RATE_44100:
    case AUDIO_SAMPLE_RATE_48000:
        i2s_fs_sel = 0x1a;
        break;
    default:
        g_critical("Sample rate %d not support\n", audio->sample_rate);
        close(fd);
        return HI_FAILURE;
    }

    if (ioctl(fd, ACODEC_SET_I2S1_FS, &i2s_fs_sel)) {
        g_critical("set acodec sample rate failed\n");
        close(fd);
        return HI_FAILURE;
    }

    unsigned int gain_mic = 0x1f;
    if (ioctl(fd, ACODEC_SET_GAIN_MICL, &gain_mic)) {
        g_warning("set acodec micin volume failed\n");
    }
    if (ioctl(fd, ACODEC_SET_GAIN_MICR, &gain_mic)) {
        g_warning("set acodec micin volume failed\n");
    }

    ACODEC_VOL_CTRL vol_ctrl;
    vol_ctrl.vol_ctrl_mute = 0;
    vol_ctrl.vol_ctrl = 0x10;
    if (ioctl(fd, ACODEC_SET_ADCL_VOL, &vol_ctrl)) {
        g_warning("set acodec adc volume failed\n");
    }
    if (ioctl(fd, ACODEC_SET_ADCR_VOL, &vol_ctrl)) {
        g_warning("set acodec adc volume failed\n");
    }

    close (fd);

	//Initialize AiDdev
	AUDIO_DEV aidev = 0;
	AIO_ATTR_S aio_attr;
	aio_attr.enSamplerate = audio->sample_rate;
	aio_attr.enBitwidth = AUDIO_BIT_WIDTH_16;
	aio_attr.enWorkmode = AIO_MODE_I2S_MASTER;
	aio_attr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	aio_attr.u32EXFlag = 1;
	aio_attr.u32FrmNum = 30;
	aio_attr.u32PtNumPerFrm = AUDIO_PTNUMPERFRM;
	aio_attr.u32ChnCnt = 2;
	aio_attr.u32ClkSel = 1;
	s32Ret = HI_MPI_AI_SetPubAttr(aidev, &aio_attr);
	if (s32Ret) {
		g_critical("failed to set AI dev%d attr\n", aidev);
		return HI_FALSE;
	}

	s32Ret = HI_MPI_AI_Enable(aidev);
	if (s32Ret) {
		g_critical("failed to enable AI dev%d\n", aidev);
		return HI_FALSE;
	}

	// Initialize AiChn
	AI_CHN aichn = 0;
	s32Ret = HI_MPI_AI_EnableChn(aidev, aichn);
	if (s32Ret) {
		g_critical("failed to enable AI chn%d-%d\n", aidev, aichn);
		return HI_FALSE;
	}

	// Initialize AENC
	AENC_CHN aencchn0 = 0;
	AENC_CHN aencchn1 = 1;
	AENC_CHN_ATTR_S aenc_attr;
	AENC_ATTR_G711_S g711_attr;

	aenc_attr.enType = PT_G711U;
	aenc_attr.u32BufSize = 30;
	aenc_attr.pValue = &g711_attr;
	g711_attr.resv = 0;

	s32Ret = HI_MPI_AENC_CreateChn(aencchn0, &aenc_attr);
	if (s32Ret) {
		g_critical("failed to create VENC chn%d\n", aencchn0);
		return HI_FALSE;
	}
	s32Ret = HI_MPI_AENC_CreateChn(aencchn1, &aenc_attr);
	if (s32Ret) {
		g_critical("failed to create VENC chn%d\n", aencchn1);
		return HI_FALSE;
	}

	// Bind AI to AENC
	MPP_CHN_S src_chn = {
		.enModId = HI_ID_AI,
		.s32DevId = 0,
		.s32ChnId = aichn
	};
	MPP_CHN_S dst0_chn = {
		.enModId = HI_ID_AENC,
		.s32DevId = 0,
		.s32ChnId = aencchn0
	};
	MPP_CHN_S dst1_chn = {
		.enModId = HI_ID_AENC,
		.s32DevId = 0,
		.s32ChnId = aencchn1
	};
	s32Ret = HI_MPI_SYS_Bind(&src_chn, &dst0_chn);
	if (s32Ret) {
		g_critical("failed to bind AENC chn%d\n", aencchn0);
		return HI_FAILURE;
	}
	s32Ret = HI_MPI_SYS_Bind(&src_chn, &dst1_chn);
	if (s32Ret) {
		g_critical("failed to bind AENC chn%d\n", aencchn1);
		return HI_FAILURE;
	}

	return 0;
}

gint32 ipcam_media_audio_stop(IpcamMediaAudio *audio)
{
	HI_S32 s32Ret;

	AUDIO_DEV aidev = 0;
	AI_CHN aichn = 0;
	AENC_CHN aencchn = 0;

	MPP_CHN_S src_chn = {
		.enModId = HI_ID_AI,
		.s32DevId = 0,
		.s32ChnId = aichn
	};
	MPP_CHN_S dst_chn = {
		.enModId = HI_ID_AENC,
		.s32DevId = 0,
		.s32ChnId = aencchn
	};
	s32Ret = HI_MPI_SYS_UnBind(&src_chn, &dst_chn);
	if (s32Ret != HI_SUCCESS) {
		g_warning("failed to unbind AENC chn%d\n", aencchn);
	}

	// AENC cleanup
	s32Ret = HI_MPI_AENC_DestroyChn(aencchn);
	if (s32Ret != HI_SUCCESS) {
		g_warning("failed to destroy AENC chn%d\n", aencchn);
	}

	// AI_CHN cleanup
	s32Ret = HI_MPI_AI_DisableChn(aidev, aichn);
	if (s32Ret != HI_SUCCESS) {
		g_warning("failed to disable AI chn%d\n", aichn);
	}

	// AI_DEV cleanup
	s32Ret = HI_MPI_AI_Disable(aidev);
	if (s32Ret != HI_SUCCESS) {
		g_warning("failed to disable AI dev%d\n", aidev);
	}

	return 0;
}

void ipcam_media_audio_param_change(IpcamMediaAudio *audio, StreamDescriptor *desc)
{
    ipcam_media_audio_stop(audio);
    ipcam_media_audio_start(audio, desc);
}

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
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

#define ENABLE_ANR			1

struct IpcamMediaAudio
{
	guint   sample_rate;
	guint   bitrate;

	gboolean terminated;
	AUDIO_DEV ai_dev;
	AI_CHN    ai_chn;
	AENC_CHN  aenc_chn[2];
	pthread_t thread;
};

IpcamMediaAudio *ipcam_media_audio_new(void)
{
	IpcamMediaAudio *audio = g_malloc0(sizeof(IpcamMediaAudio));
	audio->sample_rate = AUDIO_SAMPLE_RATE_8000;

	audio->terminated = FALSE;
	audio->ai_dev = 0;
	audio->ai_chn = 0;
	audio->aenc_chn[0] = 0;
	audio->aenc_chn[1] = 1;
	audio->thread = -1;

	return audio;
}

void ipcam_media_audio_free(IpcamMediaAudio *audio)
{
	g_free(audio);
}

/* get audio frame from ai, send it to aenc */
static void *audio_thread_handle(void *arg)
{
	IpcamMediaAudio *audio = (IpcamMediaAudio*)arg;
	HI_S32 s32Ret;
	HI_S32 AiFd;
	fd_set read_fds;
	int i;

	prctl(PR_SET_NAME, (unsigned long)"audio");

	AI_CHN_PARAM_S stAiChnParam;
	s32Ret = HI_MPI_AI_GetChnParam(audio->ai_dev, audio->ai_chn, &stAiChnParam);
	if (s32Ret != HI_SUCCESS) {
		printf("HI_MPI_AI_GetChnParam() failed\n");
		return NULL;
	}
	stAiChnParam.u32UsrFrmDepth = 30;
	s32Ret = HI_MPI_AI_SetChnParam(audio->ai_dev, audio->ai_chn, &stAiChnParam);
	if (s32Ret != HI_SUCCESS) {
		printf("HI_MPI_AI_SetChnParam() failed\n");
		return NULL;
	}

	FD_ZERO(&read_fds);
	AiFd = HI_MPI_AI_GetFd(audio->ai_dev, audio->ai_chn);
	FD_SET(AiFd, &read_fds);

	while (!audio->terminated) {
		struct timeval tout;
		tout.tv_sec = 1;
		tout.tv_usec = 0;

		FD_ZERO(&read_fds);
		FD_SET(AiFd, &read_fds);

		s32Ret = select(AiFd + 1, &read_fds, NULL, NULL, &tout);
		if (s32Ret < 0) {
			break;
		}
		else if (s32Ret == 0) {
			continue;
		}

		if (FD_ISSET(AiFd, &read_fds)) {
			AUDIO_FRAME_S stFrame;
			AEC_FRAME_S stRefFrame;
			/* get frame from ai chn */
			s32Ret = HI_MPI_AI_GetFrame(audio->ai_dev, audio->ai_chn,
			                            &stFrame, &stRefFrame, HI_FALSE);
			if (s32Ret != HI_SUCCESS) {
				printf("HI_MPI_AI_GetFrame(%d,%d) failed [%#x]\n",
				       audio->ai_dev, audio->ai_chn, s32Ret);
				continue;
			}
			/* send frame to encoder */
			for (i = 0; i < ARRAY_SIZE(audio->aenc_chn); i++) {
				HI_MPI_AENC_SendFrame(audio->aenc_chn[i], &stFrame, &stRefFrame);
			}
			/* finally release the stream */
			HI_MPI_AI_ReleaseFrame(audio->ai_dev, audio->ai_chn, &stFrame, &stRefFrame);
		}
	}

	return NULL;
}

gint32 ipcam_media_audio_start(IpcamMediaAudio *audio, StreamDescriptor *desc)
{
	HI_S32 s32Ret;
	int i;

	// Initialize Audio CODEC
	int fd = open(ACODEC_FILE, O_RDWR);
	if (fd < 0) {
		printf("Cannot open acodec %s\n", ACODEC_FILE);
		return HI_FAILURE;
	}

	if (ioctl(fd, ACODEC_SOFT_RESET_CTRL)) {
		printf("Failed to reset audio codec");
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
        printf("Sample rate %d not support\n", audio->sample_rate);
        close(fd);
        return HI_FAILURE;
    }

    if (ioctl(fd, ACODEC_SET_I2S1_FS, &i2s_fs_sel)) {
        printf("set acodec sample rate failed\n");
        close(fd);
        return HI_FAILURE;
    }

    unsigned int gain_mic = 0x1f;
    if (ioctl(fd, ACODEC_SET_GAIN_MICL, &gain_mic)) {
        printf("set acodec micin volume failed\n");
    }
    if (ioctl(fd, ACODEC_SET_GAIN_MICR, &gain_mic)) {
        printf("set acodec micin volume failed\n");
    }

    ACODEC_VOL_CTRL vol_ctrl;
    vol_ctrl.vol_ctrl_mute = 0;
    vol_ctrl.vol_ctrl = 0x08;
    if (ioctl(fd, ACODEC_SET_ADCL_VOL, &vol_ctrl)) {
        printf("set acodec adc volume failed\n");
    }
    if (ioctl(fd, ACODEC_SET_ADCR_VOL, &vol_ctrl)) {
        printf("set acodec adc volume failed\n");
    }

    close(fd);

	//Initialize AiDdev
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
	s32Ret = HI_MPI_AI_SetPubAttr(audio->ai_dev, &aio_attr);
	if (s32Ret) {
		printf("failed to set AI dev%d attr\n", audio->ai_dev);
		return HI_FALSE;
	}

	s32Ret = HI_MPI_AI_Enable(audio->ai_dev);
	if (s32Ret) {
		printf("failed to enable AI dev%d\n", audio->ai_dev);
		return HI_FALSE;
	}

	// Initialize AiChn
	s32Ret = HI_MPI_AI_EnableChn(audio->ai_dev, audio->ai_chn);
	if (s32Ret) {
		printf("failed to enable AI chn%d-%d\n", audio->ai_dev, audio->ai_chn);
		return HI_FALSE;
	}

#if defined(ENABLE_ANR)
	// Enable ANR
	s32Ret = HI_MPI_AI_EnableAnr(audio->ai_dev, audio->ai_chn);
	if (s32Ret) {
		printf("failed to enable ANR for chn%d-%d\n", audio->ai_dev, audio->ai_chn);
	}
#endif

	// Initialize AENC
	AENC_CHN_ATTR_S aenc_attr;
	AENC_ATTR_G711_S g711_attr;

	aenc_attr.enType = PT_G711U;
	aenc_attr.u32BufSize = 30;
	aenc_attr.pValue = &g711_attr;
	g711_attr.resv = 0;

	for (i = 0; i < ARRAY_SIZE(audio->aenc_chn); i++) {
		s32Ret = HI_MPI_AENC_CreateChn(audio->aenc_chn[i], &aenc_attr);
		if (s32Ret) {
			printf("failed to create VENC chn%d\n", audio->aenc_chn[i]);
			return HI_FALSE;
		}
	}

#if defined(ENABLE_ANR)
	audio->terminated = FALSE;
	pthread_create(&audio->thread, 0, audio_thread_handle, audio);
#else
	// Bind AI to AENC
	MPP_CHN_S src_chn = {
		.enModId = HI_ID_AI,
		.s32DevId = 0,
		.s32ChnId = audio->ai_chn
	};
	MPP_CHN_S dst_chn[2] = { 
		[0] = {
			.enModId = HI_ID_AENC,
			.s32DevId = 0,
			.s32ChnId = audio->aenc_chn[0]
		},
		[1] = {
			.enModId = HI_ID_AENC,
			.s32DevId = 0,
			.s32ChnId = audio->aenc_chn[1]
		}
	};
	for (i = 0; i < ARRAY_SIZE(dst_chn); i++) {
		s32Ret = HI_MPI_SYS_Bind(&src_chn, &dst_chn[i]);
		if (s32Ret) {
			printf("failed to bind AENC chn%d\n", audio->aenc_chn[i]);
			return HI_FAILURE;
		}
	}
#endif

	return 0;
}

gint32 ipcam_media_audio_stop(IpcamMediaAudio *audio)
{
	HI_S32 s32Ret;
	int i;

#if defined(ENABLE_ANR)
	audio->terminated = TRUE;
	pthread_join(audio->thread, 0);
#else
	MPP_CHN_S src_chn = {
		.enModId = HI_ID_AI,
		.s32DevId = 0,
		.s32ChnId = audio->ai_chn
	};
	MPP_CHN_S dst_chn[2] = {
		[0] = {
			.enModId = HI_ID_AENC,
			.s32DevId = 0,
			.s32ChnId = audio->aenc_chn[0]
		},
		[1] = {
			.enModId = HI_ID_AENC,
			.s32DevId = 0,
			.s32ChnId = audio->aenc_chn[1]
		}
	};
	for (i = 0; i < ARRAY_SIZE(dst_chn); i++) {
		s32Ret = HI_MPI_SYS_UnBind(&src_chn, &dst_chn[i]);
		if (s32Ret != HI_SUCCESS) {
			printf("failed to unbind AENC chn%d\n", audio->aenc_chn[i]);
		}
	}
#endif

	// AENC cleanup
	for (i = 0; i < ARRAY_SIZE(audio->aenc_chn); i++) {
		s32Ret = HI_MPI_AENC_DestroyChn(audio->aenc_chn[i]);
		if (s32Ret != HI_SUCCESS) {
			printf("failed to destroy AENC chn%d\n", audio->aenc_chn[i]);
		}
	}

#if defined(ENABLE_ANR)
	s32Ret = HI_MPI_AI_DisableAnr(audio->ai_dev, audio->ai_chn);
	if (s32Ret != HI_SUCCESS) {
		printf("failed to disable ANR for chn%d-%d\n", audio->ai_dev, audio->ai_chn);
	}
#endif

	// AI_CHN cleanup
	s32Ret = HI_MPI_AI_DisableChn(audio->ai_dev, audio->ai_chn);
	if (s32Ret != HI_SUCCESS) {
		printf("failed to disable AI chn%d\n", audio->ai_chn);
	}

	// AI_DEV cleanup
	s32Ret = HI_MPI_AI_Disable(audio->ai_dev);
	if (s32Ret != HI_SUCCESS) {
		printf("failed to disable AI dev%d\n", audio->ai_dev);
	}

	return 0;
}

void ipcam_media_audio_param_change(IpcamMediaAudio *audio, StreamDescriptor *desc)
{
    ipcam_media_audio_stop(audio);
    ipcam_media_audio_start(audio, desc);
}

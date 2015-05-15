## Process this file with automake to produce Makefile.in

## Created by Anjuta

AUTOMAKE_OPTIONS = subdir-objects

if INTERNAL_MPP
if HI3518
MPP_CFLAGS = -I$(top_srcdir)/src/hi3518/mpp2/include
MPP_LIBDIR = $(top_srcdir)/src/hi3518/mpp2/lib
endif
if HI3516
MPP_CFLAGS = -I$(top_srcdir)/src/hi3518/mpp2/include
MPP_LIBDIR = $(top_srcdir)/src/hi3518/mpp2/lib
endif
else
MPP_CFLAGS = $(MPPCFLAGS)
MPP_LIBDIR = $(MPPLIBDIR)
endif

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(IMEDIA_RTSP_CFLAGS) \
	$(MPP_CFLAGS)

AM_CFLAGS =\
	-rdynamic\
	-Wall\
	-g

bindir = $(prefix)/imedia_rtsp
bin_PROGRAMS = imedia_rtsp

imedia_rtsp_SOURCES = \
	rtsp/rtsp.cpp \
	rtsp/live/H264LiveStreamSource.cpp \
	rtsp/live/H264LiveStreamInput.cpp \
	rtsp/live/H264LiveStreamServerMediaSubsession.cpp \
	video_param_change_handler.c \
	imedia.c \
	main.c

if HI3518
imedia_rtsp_SOURCES += \
	hi3518/media_sys_ctrl.c \
	hi3518/media_video.c \
	hi3518/media_osd.c \
	hi3518/isp.c \
	hi3518/video_encode.c \
	hi3518/video_input.c \
	hi3518/video_process_subsystem.c \
	hi3518/bitmap.c \
	hi3518/video_detect.c \
	media-ircut.c \
	media-ircut.h
endif

if HI3516
imedia_rtsp_SOURCES += \
	hi3518/media_sys_ctrl.c \
	hi3518/media_video.c \
	hi3518/media_osd.c \
	hi3518/isp.c \
	hi3518/video_encode.c \
	hi3518/video_input.c \
	hi3518/video_process_subsystem.c \
	hi3518/bitmap.c \
	hi3518/video_detect.c \
	media-ircut.c \
	media-ircut.h
endif

imedia_rtsp_LDFLAGS = 

imedia_rtsp_LDADD = $(IMEDIA_RTSP_LIBS) -ldl -L$(MPP_LIBDIR) -lmpi -lmem -lisp -l_hiae -l_hiaf -l_hiawb -ltde $(MPP_LIBDIR)/libanr.a $(MPP_LIBDIR)/libaec.a $(MPP_LIBDIR)/libVoiceEngine.a $(MPP_LIBDIR)/libresampler.a $(MPP_LIBDIR)/libvqev2.a -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock

SUBDIRS = \
	config \
	hi3518/sensor

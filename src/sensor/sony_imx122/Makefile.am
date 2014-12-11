## Process this file with automake to produce Makefile.in


if INTERNAL_MPP
if HI3518
MPP_DIR = $(top_srcdir)/src/hi3518/mpp2
endif
if HI3516
MPP_DIR = $(top_srcdir)/src/hi3518/mpp2
endif
else
MPP_DIR = $(MPPDIR)
endif

AM_CPPFLAGS = \
	-I$(MPP_DIR)/include \
    -I$(MPP_DIR)/extdrv/ssp-sony

lib_LTLIBRARIES = libsns_imx122_uxga.la
libsns_imx122_uxga_la_SOURCES = imx122_cmos.c imx122_sensor_ctl.c
libsns_imx122_uxga_la_LDFLAGS = -module -export-dynamic -avoid-version

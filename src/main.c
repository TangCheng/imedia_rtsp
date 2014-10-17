/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2014 TangCheng <tangcheng2005@gmail.com>
 * 
 */

#include "imedia.h"

int main(int argc, char **argv)
{
	IpcamIMedia *imedia = g_object_new(IPCAM_IMEDIA_TYPE, "name", "imedia_rtsp", NULL);
	ipcam_base_service_start(IPCAM_BASE_SERVICE(imedia));
	return (0);
}


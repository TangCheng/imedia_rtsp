/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_awb.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2012/12/19
  Description   : 
  History       :
  1.Date        : 2012/12/19
    Author      : n00168968
    Modification: Created file

******************************************************************************/
#ifndef __MPI_AWB_H__
#define __MPI_AWB_H__

#include "hi_comm_isp.h"
#include "hi_comm_3a.h"
#include "hi_awb_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/* The interface of awb lib register to isp. */
HI_S32 HI_MPI_AWB_Register(ALG_LIB_S *pstAwbLib);

/* The callback function of sensor register to awb lib. */
HI_S32 HI_MPI_AWB_SensorRegCallBack(ALG_LIB_S *pstAwbLib, SENSOR_ID SensorId,
    AWB_SENSOR_REGISTER_S *pstRegister);

/* The new awb lib is compatible with the old mpi interface. */
HI_S32 HI_MPI_ISP_SetWBType(ISP_OP_TYPE_E enWBType);
HI_S32 HI_MPI_ISP_GetWBType(ISP_OP_TYPE_E *penWBType);

HI_S32 HI_MPI_ISP_SetAWBAttr(const ISP_AWB_ATTR_S *pstAWBAttr);
HI_S32 HI_MPI_ISP_GetAWBAttr(ISP_AWB_ATTR_S *pstAWBAttr);

HI_S32 HI_MPI_ISP_SetMWBAttr(const ISP_MWB_ATTR_S *pstMWBAttr);
HI_S32 HI_MPI_ISP_GetMWBAttr(ISP_MWB_ATTR_S *pstMWBAttr);

HI_S32 HI_MPI_ISP_SetColorTemp(const HI_U16 u16ColorTemp);     //not support yet
HI_S32 HI_MPI_ISP_GetColorTemp(HI_U16 *pu16ColorTemp);

HI_S32 HI_MPI_ISP_SetWBStaInfo(ISP_WB_STA_INFO_S *pstWBStatistic);
HI_S32 HI_MPI_ISP_GetWBStaInfo(ISP_WB_STA_INFO_S *pstWBStatistic);

HI_S32 HI_MPI_ISP_SetCCM(const ISP_COLORMATRIX_S *pstColorMatrix);
HI_S32 HI_MPI_ISP_GetCCM(ISP_COLORMATRIX_S *pstColorMatrix);

HI_S32 HI_MPI_ISP_SetSaturation(HI_U32 u32Value);
HI_S32 HI_MPI_ISP_GetSaturation(HI_U32 *pu32Value);

HI_S32 HI_MPI_ISP_SetSaturationAttr(const ISP_SATURATION_ATTR_S *pstSatAttr);
HI_S32 HI_MPI_ISP_GetSaturationAttr(ISP_SATURATION_ATTR_S *pstSatAttr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif


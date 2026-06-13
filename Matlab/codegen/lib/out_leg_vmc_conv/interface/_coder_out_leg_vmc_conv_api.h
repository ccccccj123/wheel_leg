/*
 * File: _coder_out_leg_vmc_conv_api.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 17-Dec-2023 17:05:38
 */

#ifndef _CODER_OUT_LEG_VMC_CONV_API_H
#define _CODER_OUT_LEG_VMC_CONV_API_H

/* Include Files */
#include "emlrt.h"
#include "tmwtypes.h"
#include <string.h>

/* Variable Declarations */
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
void out_leg_vmc_conv(real32_T F, real32_T Tp, real32_T phi1, real32_T phi4,
                      real32_T T[2]);

void out_leg_vmc_conv_api(const mxArray *const prhs[4], const mxArray **plhs);

void out_leg_vmc_conv_atexit(void);

void out_leg_vmc_conv_initialize(void);

void out_leg_vmc_conv_terminate(void);

void out_leg_vmc_conv_xil_shutdown(void);

void out_leg_vmc_conv_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_out_leg_vmc_conv_api.h
 *
 * [EOF]
 */

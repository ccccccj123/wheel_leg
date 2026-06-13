/*
 * File: _coder_out_leg_speed_api.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 12-Dec-2023 12:35:07
 */

#ifndef _CODER_OUT_LEG_SPEED_API_H
#define _CODER_OUT_LEG_SPEED_API_H

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
void out_leg_speed(real32_T d_phi1, real32_T d_phi4, real32_T phi1,
                   real32_T phi4, real32_T speed[2]);

void out_leg_speed_api(const mxArray *const prhs[4], const mxArray **plhs);

void out_leg_speed_atexit(void);

void out_leg_speed_initialize(void);

void out_leg_speed_terminate(void);

void out_leg_speed_xil_shutdown(void);

void out_leg_speed_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_out_leg_speed_api.h
 *
 * [EOF]
 */

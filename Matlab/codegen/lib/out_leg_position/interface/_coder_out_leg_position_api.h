/*
 * File: _coder_out_leg_position_api.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 17-Dec-2023 22:40:34
 */

#ifndef _CODER_OUT_LEG_POSITION_API_H
#define _CODER_OUT_LEG_POSITION_API_H

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
void out_leg_position(real32_T phi1, real32_T phi4, real32_T pos[2]);

void out_leg_position_api(const mxArray *const prhs[2], const mxArray **plhs);

void out_leg_position_atexit(void);

void out_leg_position_initialize(void);

void out_leg_position_terminate(void);

void out_leg_position_xil_shutdown(void);

void out_leg_position_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_out_leg_position_api.h
 *
 * [EOF]
 */

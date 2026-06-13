/*
 * File: _coder_out_lqr_k_api.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 10-Apr-2024 09:54:10
 */

#ifndef _CODER_OUT_LQR_K_API_H
#define _CODER_OUT_LQR_K_API_H

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
void out_lqr_k(real32_T L0, real32_T K[12]);

void out_lqr_k_api(const mxArray *prhs, const mxArray **plhs);

void out_lqr_k_atexit(void);

void out_lqr_k_initialize(void);

void out_lqr_k_terminate(void);

void out_lqr_k_xil_shutdown(void);

void out_lqr_k_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_out_lqr_k_api.h
 *
 * [EOF]
 */

/*
 * File: _coder_out_leg_speed_mex.h
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 12-Dec-2023 12:35:07
 */

#ifndef _CODER_OUT_LEG_SPEED_MEX_H
#define _CODER_OUT_LEG_SPEED_MEX_H

/* Include Files */
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
MEXFUNCTION_LINKAGE void mexFunction(int32_T nlhs, mxArray *plhs[],
                                     int32_T nrhs, const mxArray *prhs[]);

emlrtCTX mexFunctionCreateRootTLS(void);

void unsafe_out_leg_speed_mexFunction(int32_T nlhs, mxArray *plhs[1],
                                      int32_T nrhs, const mxArray *prhs[4]);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_out_leg_speed_mex.h
 *
 * [EOF]
 */

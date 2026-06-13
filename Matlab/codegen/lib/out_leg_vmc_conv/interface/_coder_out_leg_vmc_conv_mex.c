/*
 * File: _coder_out_leg_vmc_conv_mex.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 17-Dec-2023 17:05:38
 */

/* Include Files */
#include "_coder_out_leg_vmc_conv_mex.h"
#include "_coder_out_leg_vmc_conv_api.h"

/* Function Definitions */
/*
 * Arguments    : int32_T nlhs
 *                mxArray *plhs[]
 *                int32_T nrhs
 *                const mxArray *prhs[]
 * Return Type  : void
 */
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&out_leg_vmc_conv_atexit);
  /* Module initialization. */
  out_leg_vmc_conv_initialize();
  /* Dispatch the entry-point. */
  unsafe_out_leg_vmc_conv_mexFunction(nlhs, plhs, nrhs, prhs);
  /* Module termination. */
  out_leg_vmc_conv_terminate();
}

/*
 * Arguments    : void
 * Return Type  : emlrtCTX
 */
emlrtCTX mexFunctionCreateRootTLS(void)
{
  emlrtCreateRootTLSR2021a(&emlrtRootTLSGlobal, &emlrtContextGlobal, NULL, 1,
                           NULL);
  return emlrtRootTLSGlobal;
}

/*
 * Arguments    : int32_T nlhs
 *                mxArray *plhs[1]
 *                int32_T nrhs
 *                const mxArray *prhs[4]
 * Return Type  : void
 */
void unsafe_out_leg_vmc_conv_mexFunction(int32_T nlhs, mxArray *plhs[1],
                                         int32_T nrhs, const mxArray *prhs[4])
{
  emlrtStack st = {
      NULL, /* site */
      NULL, /* tls */
      NULL  /* prev */
  };
  const mxArray *outputs;
  st.tls = emlrtRootTLSGlobal;
  /* Check for proper number of arguments. */
  if (nrhs != 4) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 4, 4,
                        16, "out_leg_vmc_conv");
  }
  if (nlhs > 1) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 16,
                        "out_leg_vmc_conv");
  }
  /* Call the function. */
  out_leg_vmc_conv_api(prhs, &outputs);
  /* Copy over outputs to the caller. */
  emlrtReturnArrays(1, &plhs[0], &outputs);
}

/*
 * File trailer for _coder_out_leg_vmc_conv_mex.c
 *
 * [EOF]
 */

/*
 * File: out_lqr_k_initialize.c
 *
 * MATLAB Coder version            : 5.3
 * C/C++ source code generated on  : 10-Apr-2024 09:54:10
 */

/* Include Files */
#include "out_lqr_k_initialize.h"
#include "out_lqr_k_data.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : void
 * Return Type  : void
 */
void out_lqr_k_initialize(void)
{
  rt_InitInfAndNaN();
  isInitialized_out_lqr_k = true;
}

/*
 * File trailer for out_lqr_k_initialize.c
 *
 * [EOF]
 */

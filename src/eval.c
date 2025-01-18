#define EVALTSCTIME
#include "measurement.h"
#include <stdio.h>

int main(void) {
  measurement_t t = eval_tsc_cycles();
  printf("moyenne=%f\tmin=%f\tmax=%f\tvariance=%f\n", t.tavg, t.tmin, t.tmax,
         t.tvar);
  return 0;
}

#define EVALTSCTIME
#include "measurement.h"
#include <stdio.h>

int main(void) {
  measurement t = eval_tsc_cycles();
  printf("moyenne=%f\tmin=%f\tmax=%f\tvariance=%f\tavg=%f\n", t.tavg, t.tmin,
         t.tmax, t.tvar, t.ttavg);
  return 0;
}

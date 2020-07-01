#include <iostream>
#include "perf_counters.h"
using namespace std;
int main() {
  PCM_initPerformanceMonitor(NULL, NULL);
  PCM_start();


  PCM_stop();
  PCM_log("========== profiling results ==========\n");
  PCM_printResults();
  PCM_log("=======================================\n");
  PCM_cleanup();

  return 0;
}

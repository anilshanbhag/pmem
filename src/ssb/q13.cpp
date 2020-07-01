// Ensure printing of CUDA runtime errors to console
#define CUB_STDERR

#include <iostream>
#include <stdio.h>
#include <chrono>

#include "tbb/tbb.h"
#include "tbb/parallel_for.h"

#include "../utils/cpu_utils.h"
#include "ssb_utils.h"

using namespace std;
using namespace tbb;

/*
Implementation of Q13
select sum(lo_extendedprice * lo_discount) as revenue from lineorder,date where lo_orderdate = d_datekey and d_weeknuminyear = 6 and d_year = 1994 and lo_discount>=5 and lo_discount<=7 and lo_quantity>=26 and lo_quantity<=35;
*/

float runQuery(int* lo_orderdate, int* lo_discount, int* lo_quantity, int* lo_extendedprice, size_t num_items) {
  chrono::high_resolution_clock::time_point start, finish;
  start = chrono::high_resolution_clock::now();

  tbb::atomic<unsigned long long> revenue = 0;

  parallel_for(blocked_range<size_t>(0, num_items, num_items/NUM_THREADS + 4), [&](auto range) {
    size_t start = range.begin();
    size_t end = range.end();
    size_t end_batch = start + ((end - start)/BATCH_SIZE) * BATCH_SIZE;
    unsigned long long local_revenue = 0;
    for (size_t batch_start = start; batch_start < end_batch; batch_start += BATCH_SIZE) {
      bool selection_flag;
      #pragma simd
      for (size_t i = batch_start; i < batch_start + BATCH_SIZE; i++) {
        if (lo_orderdate[i] >= 19940204 && lo_orderdate[i] <= 19940210)
          if (lo_quantity[i] >= 26 && lo_quantity[i] <= 35)
            if (lo_discount[i] >= 5 && lo_discount[i] <= 7)
              local_revenue += (lo_extendedprice[i] * lo_discount[i]);
      }
    }

    bool selection_flag;
    for (size_t i = end_batch; i < end; i++) {
      selection_flag = (lo_orderdate[i] >= 19940204 && lo_orderdate[i] <= 19940210);
      selection_flag = selection_flag && (lo_quantity[i] >= 26 && lo_quantity[i] <= 35);
      selection_flag = selection_flag && (lo_discount[i] >= 5 && lo_discount[i] <= 7);
      //local_revenue += selection_flag;
      local_revenue += selection_flag * (lo_extendedprice[i] * lo_discount[i]);
    }
    revenue.fetch_and_add(local_revenue);
  });

  finish = chrono::high_resolution_clock::now();

  cout << "Revenue: " << revenue << endl;

  std::chrono::duration<double> diff = finish - start;
  return diff.count() * 1000;
}

/**
 * Main
 */
int main(int argc, char** argv) {
  int num_trials = 3;
  string type    = "mem"; // | pmem

  // Initialize command line
  CommandLineArgs args(argc, argv);
  args.GetCmdLineArgument("t", num_trials);
  args.GetCmdLineArgument("m", type);

  // Print usage
  if (args.CheckCmdLineFlag("help")) {
    printf("%s "
      "[--t=<num trials>] "
      "[--m=<memory type>] "
      "[--v] "
      "\n", argv[0]);
    exit(0);
  }

  // Load in data
  int *lo_orderdate = loadColumn<int>("lo_orderdate", LO_LEN, type);
  int *lo_discount = loadColumn<int>("lo_discount", LO_LEN, type);
  int *lo_quantity = loadColumn<int>("lo_quantity", LO_LEN, type);
  int *lo_extendedprice = loadColumn<int>("lo_extendedprice", LO_LEN, type);

  cout << "** LOADED DATA **" << endl;

  // For selection: Initally assume everything is selected
  //bool *selection_flags = (bool*) malloc(sizeof(bool) * LO_LEN);
  //for (size_t i = 0; i < LO_LEN; i++) {
    //selection_flag = true;
  //}

  // Run trials
  for (int t = 0; t < num_trials; t++) {
    float time_query = runQuery(lo_orderdate,
                                lo_discount,
                                lo_quantity,
                                lo_extendedprice,
                                LO_LEN);

    cout << "{" << "\"query\":13" << ",\"time_query\":" << time_query << "}" << endl;
  }

  return 0;
}

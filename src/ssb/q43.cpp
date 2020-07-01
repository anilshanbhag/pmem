// Ensure printing of CUDA runtime errors to console
#define CUB_STDERR

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <atomic>

#include "tbb/tbb.h"
#include "tbb/parallel_for.h"

#include "../utils/cpu_utils.h"
#include "ssb_utils.h"

using namespace std;
using namespace tbb;

#define HASH_WM(X,Y,Z) ((X-Z) % Y)
#define HASH(X,Y) (X % Y)

struct slot {
  int year;
  int s_nation;
  int category;
  std::atomic<int> revenue;
};

/*
Implementation of q43
select d_year,s_city,p_brand1,sum(lo_revenue-lo_supplycost) as profit from lineorder,supplier,customer,part,date where lo_custkey = c_custkey and lo_suppkey = s_suppkey and lo_partkey = p_partkey and lo_orderdate = d_datekey and c_region = 'AMERICA' and s_nation = 'UNITED STATES' and (d_year = 1997 or d_year = 1998) and p_category = 'MFGR#14' group by d_year,s_city,p_brand1 order by d_year,s_city,p_brand1;
*/

float runQuery(int* lo_orderdate, int* lo_custkey, int* lo_partkey, int* lo_suppkey, int* lo_revenue, int* lo_supplycost, size_t lo_len,
    int *d_datekey, int *d_year, int d_len,
    int *p_partkey, int *p_category, int *p_brand1, int p_len,
    int *s_suppkey, int* s_nation, int* s_city, int s_len,
    int *c_custkey, int* c_region, int c_len) {
  chrono::high_resolution_clock::time_point start, finish;
  start = chrono::high_resolution_clock::now();

  int d_val_len = 19981230 - 19920101 + 1;
  int d_val_min = 19920101;
  int *ht_d = (int*)malloc(2 * d_val_len * sizeof(int));
  int *ht_s = (int*)malloc(2 * s_len * sizeof(int));
  int *ht_c = (int*)malloc(2 * c_len * sizeof(int));
  int *ht_p = (int*)malloc(2 * p_len * sizeof(int));

  memset(ht_d, 0, 2 * d_val_len * sizeof(int));
  memset(ht_s, 0, 2 * s_len * sizeof(int));
  memset(ht_c, 0, 2 * c_len * sizeof(int));
  memset(ht_p, 0, 2 * p_len * sizeof(int));

  // Build hashtable d
  parallel_for(blocked_range<size_t>(0, d_len, d_len/NUM_THREADS + 4), [&](auto range) {
    for (int i = range.begin(); i < range.end(); i++) {
      if (d_year[i] == 1997 || d_year[i] == 1998) {
        int key = d_datekey[i];
        int hash = HASH_WM(key, d_val_len, d_val_min);
        ht_d[hash << 1] = key;
        ht_d[(hash << 1) + 1] = d_year[i];
      }
    }
  });

  // Build hashtable c
  parallel_for(blocked_range<size_t>(0, c_len, c_len/NUM_THREADS + 4), [&](auto range) {
    for (int i = range.begin(); i < range.end(); i++) {
      if (c_region[i] == 1) {
        int key = c_custkey[i];
        int hash = HASH(key, c_len);
        ht_c[hash << 1] = key;
      }
    }
  });

  // Build hashtable s
  parallel_for(blocked_range<size_t>(0, s_len, s_len/NUM_THREADS + 4), [&](auto range) {
    for (int i = range.begin(); i < range.end(); i++) {
      if (s_nation[i] == 24) {
        int key = s_suppkey[i];
        int val = s_city[i];
        int hash = HASH(key, s_len);
        ht_s[hash << 1] = key;
        ht_s[(hash << 1) + 1] = val;
      }
    }
  });

  // Build hashtable p
  parallel_for(blocked_range<size_t>(0, p_len, p_len/NUM_THREADS + 4), [&](auto range) {
    for (int i = range.begin(); i < range.end(); i++) {
      if (p_category[i] == 3) {
        int key = p_partkey[i];
        int val = p_brand1[i];
        int hash = HASH(key, p_len);
        ht_p[hash << 1] = key;
        ht_p[(hash << 1) + 1] = val;
      }
    }
  });

  int num_slots = ((1998-1992+1) * 250 * 1000);
  slot* res = new slot[num_slots];

  for (int i=0; i<num_slots; i++) {
    res[i].year = 0;
  }

  // Probe
  parallel_for(blocked_range<size_t>(0, lo_len, lo_len/NUM_THREADS + 4), [&](auto range) {
    size_t start = range.begin();
    size_t end = range.end();
    size_t end_batch = start + ((end - start)/BATCH_SIZE) * BATCH_SIZE;
    unsigned long long local_revenue = 0;
    for (size_t batch_start = start; batch_start < end_batch; batch_start += BATCH_SIZE) {
      #pragma simd
      for (size_t i = batch_start; i < batch_start + BATCH_SIZE; i++) {
        int hash = HASH(lo_suppkey[i], s_len);
        long long slot = reinterpret_cast<long long *>(ht_s)[hash];
        if (slot != 0) {
          int s_city = slot >> 32;
          hash = HASH(lo_custkey[i], c_len);
          slot = ht_c[hash << 1];
          if (slot != 0) {
            hash = HASH(lo_partkey[i], p_len);
            slot = ht_p[hash << 1];
            if (slot != 0) {
              int brand = ht_p[(hash << 1) + 1];
              hash = HASH_WM(lo_orderdate[i], d_val_len, d_val_min);
              slot = ht_d[hash << 1];
              if (slot != 0) {
                int year = ht_d[(hash << 1) + 1];
                hash = ((year - 1992) * 250 * 1000 + s_city * 1000 + brand) % num_slots;
                res[hash].year = year;
                res[hash].s_nation = s_city;
                res[hash].category = brand;
                res[hash].revenue += lo_revenue[i] - lo_supplycost[i];
              }
            }
          }
        }
      }
    }
    for (size_t i = end_batch ; i < end; i++) {
      int hash = HASH(lo_suppkey[i], s_len);
      int slot = ht_s[hash << 1];
      if (slot != 0) {
        int s_nation = ht_s[(hash << 1) + 1];
        hash = HASH(lo_custkey[i], c_len);
        slot = ht_c[hash << 1];
        if (slot != 0) {
          hash = HASH(lo_partkey[i], p_len);
          slot = ht_p[hash << 1];
          if (slot != 0) {
            int category = ht_p[(hash << 1) + 1];
            hash = HASH_WM(lo_orderdate[i], d_val_len, d_val_min);
            slot = ht_d[hash << 1];
            if (slot != 0) {
              int year = ht_d[(hash << 1) + 1];
              hash = ((year - 1992) * 25 * 5540 + s_nation * 250 + category) % num_slots;
              res[hash].year = year;
              res[hash].s_nation = s_nation;
              res[hash].category = category;
              res[hash].revenue += lo_revenue[i] - lo_supplycost[i];
            }
          }
        }
      }
    }
  });

  finish = chrono::high_resolution_clock::now();

  cout << "Result:" << endl;

  int res_count = 0;
  for (int i=0; i<num_slots; i++) {
    if (res[i].year != 0) {
      cout << res[i].year << " " << res[i].s_nation << " " << res[i].category << " " << res[i].revenue << endl;
      res_count += 1;
    }
  }

  cout << "Res Count: " << res_count << endl;

  free(ht_c); free(ht_p); free(ht_d); free(ht_s);
  delete[] res;

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
  int *lo_custkey = loadColumn<int>("lo_custkey", LO_LEN, type);
  int *lo_suppkey = loadColumn<int>("lo_suppkey", LO_LEN, type);
  int *lo_partkey = loadColumn<int>("lo_partkey", LO_LEN, type);
  int *lo_revenue = loadColumn<int>("lo_revenue", LO_LEN, type);
  int *lo_supplycost = loadColumn<int>("lo_supplycost", LO_LEN, type);

  int *d_datekey = loadColumn<int>("d_datekey", D_LEN);
  int *d_year = loadColumn<int>("d_year", D_LEN);

  int *s_suppkey = loadColumn<int>("s_suppkey", S_LEN);
  int *s_city = loadColumn<int>("s_city", S_LEN);
  int *s_nation = loadColumn<int>("s_nation", S_LEN);

  int *p_partkey = loadColumn<int>("p_partkey", P_LEN);
  int *p_brand1 = loadColumn<int>("p_brand1", P_LEN);
  int *p_category = loadColumn<int>("p_category", P_LEN);

  int *c_custkey = loadColumn<int>("c_custkey", C_LEN);
  int *c_region = loadColumn<int>("c_region", C_LEN);

  cout << "** LOADED DATA **" << endl;

  // For selection: Initally assume everything is selected
  bool *selection_flags = (bool*) malloc(sizeof(bool) * LO_LEN);
  for (size_t i = 0; i < LO_LEN; i++) {
    selection_flags[i] = true;
  }

  // Run trials
  for (int t = 0; t < num_trials; t++) {
    float time_query;
    time_query = runQuery(
        lo_orderdate, lo_custkey, lo_partkey, lo_suppkey, lo_revenue, lo_supplycost, LO_LEN,
        d_datekey, d_year, D_LEN,
        p_partkey, p_category, p_brand1, P_LEN,
        s_suppkey, s_nation, s_city, S_LEN,
        c_custkey, c_region, C_LEN);
    cout<< "{"
        << "\"query\":43"
        << ",\"time_query\":" << time_query
        << "}" << endl;
  }

  return 0;
}

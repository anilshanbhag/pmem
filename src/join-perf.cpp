#include <math.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <fstream>
#include <libpmem.h>

#include "tbb/tbb.h"
#include "tbb/parallel_for.h"

#include "utils/generator.h"
#include "utils/cpu_utils.h"

#include "perf_counters.h"

using namespace tbb;
using namespace std;

void *mamalloc(size_t size)
{
  void *ptr = NULL;
  return posix_memalign(&ptr, 64, size) ? NULL : ptr;
}

int *align(const int* p)
{
  while (63 & (size_t) p) p++;
  return (int*) p;
}

int log_2(size_t n)
{
  size_t b = 1;
  int p = 0;
  while (b < n) {
    b += b;
    p++;
  }
  assert(b == n);
  return p;
}

typedef struct {
  uint32_t key;
  uint32_t val;
} bucket_t;

float build(const uint32_t *keys, const uint32_t *vals, size_t size, bucket_t *table,
           int num_slots)
{
  chrono::high_resolution_clock::time_point start, finish;
  const int batch_size = 1<<22;

  start = chrono::high_resolution_clock::now();
  parallel_for(blocked_range<size_t>(0, size, batch_size), [&](auto range) {
    size_t i;
    for (i = range.begin(); i != range.end(); ++i) {
      uint32_t key = keys[i];
      uint32_t val = vals[i];
      size_t h = (key & (num_slots - 1));
      //while (table[h].key != invalid_key)
        //h = (h + 1) & (buckets - 1);
      table[h].key = key;
      table[h].val = val;
    }
  });

  finish = chrono::high_resolution_clock::now();

  std::chrono::duration<double> diff = finish - start;
  return diff.count() * 1000;
}

inline void store(uint32_t *p, uint32_t v)
{
#ifndef __MIC
  _mm_stream_si32((int*) p, v);
#else
  *p = v;
#endif
}

float probe(uint32_t *keys, uint32_t *vals, size_t size,
              const bucket_t *table, int num_slots)
{
  chrono::high_resolution_clock::time_point start, finish;
  const int batch_size = 1<<22;
  const uint32_t invalid_key = 0;
  tbb::atomic<long long> full_checksum;
  full_checksum = 0;

  start = chrono::high_resolution_clock::now();

  parallel_for(blocked_range<size_t>(0, size, batch_size), [&](auto range) {
    size_t i;
    long long checksum = 0;
    const uint64_t *table_64 = (const uint64_t*) table;
    for (i = range.begin(); i != range.end(); ++i) {
      uint32_t key = keys[i];
      uint32_t val = vals[i];
      size_t h = (uint32_t) (key & (num_slots - 1));
      //size_t h = HASH(key, num_slots);
      uint64_t tab = table_64[h];
      if (key == (uint32_t) tab) {
        checksum += val + (tab >>  32);
        //cout << "K V1 V2 " << key << " " << val << " " << (tab >> 32) << endl;
      } else while (invalid_key != (uint32_t) tab) {
        h = (h + 1) & (num_slots - 1);
        tab = table_64[h];
        if (key == (uint32_t) tab) {
          checksum += val + (tab >> 32);
          break;
        }
      }
    }
    full_checksum.fetch_and_add(checksum);
  });

  finish = chrono::high_resolution_clock::now();

  long long res_checksum = full_checksum.fetch_and_add(0);
  cout << "Checksum Scalar " << res_checksum << endl;

  std::chrono::duration<double> diff = finish - start;
  return diff.count() * 1000;
}

struct TimeKeeper {
  float time_build;
  float time_probe;
  float time_extra;
  float time_total;
};

float parallel_zero(int *arr, int size) {
  chrono::high_resolution_clock::time_point start, finish;
  start = chrono::high_resolution_clock::now();
  const __m256i zero = _mm256_set1_epi32(0);
  int batch_size = 1<<21;

  parallel_for(blocked_range<size_t>(0, size, batch_size), [&](auto range) {
    for (size_t i = range.begin(); i < range.end(); i += 8) {
      _mm256_stream_si256((__m256i*) &arr[i], zero);
    }
  });
  finish = chrono::high_resolution_clock::now();

  std::chrono::duration<double> diff = finish - start;
  return diff.count() * 1000;
}

TimeKeeper joinScalar(uint32_t* dim_key, uint32_t* dim_val, uint32_t* fact_fkey, uint32_t* fact_val, int num_dim, int num_fact) {
  bucket_t* hash_table = NULL;
  int num_slots = num_dim;
  float time_build, time_probe, time_memset, time_memset2;

  hash_table = (bucket_t*) _mm_malloc(sizeof(bucket_t) * num_slots, 256);

  time_memset = parallel_zero((int*)hash_table, num_slots * 2);

  time_build = build(dim_key, dim_val, num_dim, hash_table, num_slots);

  PCM_start();

  time_probe = probe(fact_fkey, fact_val, num_fact, hash_table, num_slots);

  PCM_stop();
  PCM_log("========== profiling results ==========\n");
  PCM_printResults();
  PCM_log("=======================================\n");
  PCM_cleanup();

  TimeKeeper t = {time_build, time_probe, time_memset, time_build + time_probe + time_memset};
  return t;
}

inline bool file_exists (const std::string& name) {
  ifstream f(name.c_str());
  return f.good();
}

uint32_t* create_int_pmem_buffer(string name, int num_items) {
  size_t buffer_size = sizeof(int) * num_items;
  string path = "/mnt/pmem12/";
  path += name;

  char* pmemaddr;
  int is_pmem;
  size_t mapped_len;
  if (file_exists(path)) {
    pmemaddr = (char*)pmem_map_file(path.c_str(), buffer_size,
        PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
  } else {
    pmemaddr = (char*)pmem_map_file(path.c_str(), buffer_size,
        PMEM_FILE_CREATE|PMEM_FILE_EXCL, 0666, &mapped_len, &is_pmem);
  }

  if (pmemaddr == NULL) {
    perror("pmem_map_file");
    exit(1);
  }

  if (is_pmem) {
    cout << "Pmem successfully allocated" << endl;
  } else {
    cout << "Error not pmem" << endl;
    exit(1);
  }

  uint32_t* buf = reinterpret_cast<uint32_t*>(pmemaddr);
  return buf;
}

void pmem_memcpy(uint32_t* dst, uint32_t* src, int num_items) {
  for (int i=0; i<num_items; i++) {
    dst[i] = src[i];
  }
}


int main(int argc, char** argv)
{
  int num_fact     = 256 * 1<<20;
  int num_dim      = 64 * 1<<20;
  int num_trials   = 3;
  int device       = 1;

  // Initialize command line
  CommandLineArgs args(argc, argv);
  args.GetCmdLineArgument("n", num_fact);
  args.GetCmdLineArgument("d", num_dim);
  args.GetCmdLineArgument("t", num_trials);
  args.GetCmdLineArgument("device", device);

  // Print usage
  if (args.CheckCmdLineFlag("help"))
  {
    printf("%s "
      "[--n=<num fact>] "
      "[--d=<num dim>] "
      "[--t=<num trials>] "
      "[--device=<0=>nvm 1=>ram>] "
      "[--v] "
      "\n", argv[0]);
    exit(0);
  }

  int *h_dim_key = NULL;
  int *h_dim_val = NULL;
  int *h_fact_fkey = NULL;
  int *h_fact_val = NULL;

  create_relation_pk(h_dim_key, h_dim_val, num_dim);
  create_relation_fk(h_fact_fkey, h_fact_val, num_fact, num_dim);

  long long sum = 0;
  for (int i=0; i<num_fact; i++) sum += h_fact_val[i];
  //cout << "FSUM " << sum << endl;

  for (int i=0; i<num_dim; i++) sum += h_dim_val[i];
  //cout << "DSUM " << sum << endl;

  cout << "Done Generating" << endl;

  PCM_initPerformanceMonitor("/root/nvm_perf/pcm.cfg", NULL);

  uint32_t *dim_key, *dim_val, *fact_fkey, *fact_val;
  if (device == 0) {
    cout << "Allocating on PMEM" << endl;
    dim_key = create_int_pmem_buffer("dim_key", num_dim);
    dim_val = create_int_pmem_buffer("dim_val", num_dim);
    fact_fkey = create_int_pmem_buffer("fact_fkey", num_fact);
    fact_val = create_int_pmem_buffer("fact_val", num_fact);
    pmem_memcpy(dim_key, (uint32_t*)h_dim_key, num_dim);
    cout << "Copied to PMEM" << endl;
    pmem_memcpy(dim_val, (uint32_t*)h_dim_val, num_dim);
    cout << "Copied to PMEM" << endl;
    pmem_memcpy(fact_fkey, (uint32_t*)h_fact_fkey, num_fact);
    cout << "Copied to PMEM" << endl;
    pmem_memcpy(fact_val, (uint32_t*)h_fact_val, num_fact);
    cout << "Copied to PMEM" << endl;
  } else {
    dim_key = (uint32_t*) h_dim_key;
    dim_val = (uint32_t*) h_dim_val;
    fact_fkey = (uint32_t*) h_fact_fkey;
    fact_val = (uint32_t*) h_fact_val;
  }

  for (int t = 0; t < num_trials; t++) {
    TimeKeeper time_scalar = joinScalar(dim_key, dim_val, fact_fkey, fact_val, num_dim, num_fact);

    cout<< "{"
      << "\"num_dim\":" << num_dim
      << ",\"num_fact\":" << num_fact
      << ",\"time_build_scalar\":" << time_scalar.time_build
      << ",\"time_probe_scalar\":" << time_scalar.time_probe
      << "}" << endl;
  }

  return 0;
}


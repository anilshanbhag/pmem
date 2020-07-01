#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "tbb/tbb.h"
#include "tbb/parallel_for.h"

#include "utils/cpu_utils.h"
#include <libpmem.h>

using namespace tbb;
using namespace std;

double streaming_read(int* key_buf, int num_items) {
  chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
  int batch_size = 1<<25;
  __m256i global_counter = _mm256_set1_epi32(0);
  while (1) {
    parallel_for(blocked_range<size_t>(0, num_items, batch_size), [&](auto range) {
      int count = 0;
      __m256i counter = _mm256_set1_epi32(0);
      for (int j=0; j<1000; j++) {
        for (size_t i = range.begin(); i < range.end(); i += 8) {
          //if (i%8 != 0) cout << i << endl;
          __m256i load = _mm256_load_si256((__m256i*)&key_buf[i]);
          counter = _mm256_add_epi32(load, counter);
        }
      }
      global_counter = _mm256_add_epi32(counter, global_counter);
    });
  }
  chrono::high_resolution_clock::time_point finish = chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = finish - start;
  int t = _mm256_extract_epi32(global_counter, 0);
  return diff.count() * 1000000.0;
}

int main(int argc, char** argv)
{
    uint num_items          = 1 << 30;
    int num_trials          = 3;
    bool full_agg           = true;

    // Initialize command line
    CommandLineArgs args(argc, argv);
    args.GetCmdLineArgument("n", num_items);
    args.GetCmdLineArgument("t", num_trials);

    // Print usage
    if (args.CheckCmdLineFlag("help"))
    {
        printf("%s "
            "[--n=<input items>] "
            "[--t=<num trials>] "
            "[--v] "
            "\n", argv[0]);
        exit(0);
    }

    int *h_keys;
    // h_keys = (int*) _mm_malloc(sizeof(int) * num_items, 256);

    char *pmemaddr;
    size_t mapped_len;
    int is_pmem;

    /* create a pmem file and memory map it */
    size_t buffer_size = sizeof(int) * num_items;
    char *path = "/mnt/pmem12/loop_buffer";
    if ((pmemaddr = (char*)pmem_map_file(path, buffer_size, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL) {
        perror("pmem_map_file");
         exit(1);
    }

    h_keys = reinterpret_cast<int*>(pmemaddr);

    parallel_for(blocked_range<size_t>(0, num_items, 32 * 1024), [&](auto range) {
      unsigned int seed = range.begin();
      for (size_t i = range.begin(); i < range.end(); i++) {
        h_keys[i] = rand_r(&seed) & 15;
      }
    });


    for (int i = 0; i < num_trials; i++) {
        // Full Aggregation.
        double time_taken;
        double bandwidth;

        time_taken = streaming_read(h_keys, num_items);
        bandwidth = (num_items * 4) / time_taken;

        cout<< "{"
            << "\"time_taken\":" << time_taken << ","
            << "\"streaming_read_bandwidth\":" << bandwidth
            << "}" << endl;
    }

    pmem_unmap(pmemaddr, mapped_len);

    if (remove(path) != 0)
        perror("Error deleting file");

    return 0;
}

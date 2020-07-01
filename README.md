# SQL Analytics on Persistent Memory

New data storage technologies such as the recently introduced Intel® Optane™ DC Persistent Memory Module (PMM) offer exciting opportunities for optimizing the query processing performance of database workloads. In particular, the unique combination of low latency, byte-addressability, persistence, and large capacity make persistent memory (PMem) an attractive alternative along with DRAM and SSDs. In this work, we present one of the first experimental studies on characterizing Intel® Optane™ DC PMM's performance behavior in the context of analytical database workloads. 

The repo contains:

* `src/` contains implementations of sequential, selective, and random access patterns
* `src/ssb` contains Implementations of 13 queries of the Star Schema Benchmark

For full details checkout our [paper](http://anilshanbhag.in/static/papers/crystal_sigmod20.pdf)

```
@inproceedings{shanbhag2020pmem,
  author = {Shanbhag, Anil and Tatbul, Nesime and Cohen, David and Madden, Samuel},
  title = {Large-Scale in-Memory Analytics on Intel® OptaneTM DC Persistent Memory},
  year = {2020},
  url = {https://doi.org/10.1145/3399666.3399933},
  doi = {10.1145/3399666.3399933},
  booktitle = {Proceedings of the 16th International Workshop on Data Management on New Hardware},
  articleno = {4},
  numpages = {8},
  location = {Portland, Oregon},
  series = {DaMoN ’20}
}
```

Usage
----

Make sure you have the following package installed:

* `intel tbb`
* [`pmdk`](https://github.com/pmem/pmdk)

If you want to see how to read and write to PMem, check `src/utils/cpu_utils.h`

To run the microbenchmarks:

* `make bin/select`
* `./bin/select` to run on DRAM
* `./bin/select --m=pmem` to run on PMem

Look at `experiments` for script files to run microbenchmarks.

To run the Star Schema Benchmark implementation:

* Generate the test dataset

```
cd test/

# Generate the test generator / transformer binaries
cd ssb/dbgen
make
cd ../loader
make 
cd ../../

# Generate the test data and transform into columnar layout
# Substitute <SF> with appropriate scale factor (eg: 1)
python util.py ssb <SF> gen
python util.py ssb <SF> transform
```

* Configure the benchmark settings
```
cd src/ssb/
# Edit SF and BASE_PATH in ssb_utils.h
```

* To run a query, say run q11
```
make bin/ssb/q11
./bin/ssb/q11 
./bin/ssb/q11 --m=pmem
```


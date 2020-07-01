#!/bin/bash

source config.sh
LOG_DIR=../logs/$MACHINE
SF=1000

mkdir -p $LOG_DIR/cpu/ssb/$SF/

numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q11 --m=pmem > $LOG_DIR/ssb/$SF/n11
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q12 --m=pmem > $LOG_DIR/ssb/$SF/n12
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q13 --m=pmem > $LOG_DIR/ssb/$SF/n13
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q21 --m=pmem > $LOG_DIR/ssb/$SF/n21
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q22 --m=pmem > $LOG_DIR/ssb/$SF/n22
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q23 --m=pmem > $LOG_DIR/ssb/$SF/n23
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q31 --m=pmem > $LOG_DIR/ssb/$SF/n31
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q32 --m=pmem > $LOG_DIR/ssb/$SF/n32
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q33 --m=pmem > $LOG_DIR/ssb/$SF/n33
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q34 --m=pmem > $LOG_DIR/ssb/$SF/n34
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q41 --m=pmem > $LOG_DIR/ssb/$SF/n41
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q42 --m=pmem > $LOG_DIR/ssb/$SF/n42
numactl --physcpubind=24-47,72-95 --membind=1 ../bin/ssb/q43 --m=pmem > $LOG_DIR/ssb/$SF/n43



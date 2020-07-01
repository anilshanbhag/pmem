#!/bin/bash

source config.sh
LOG_DIR=../logs/$MACHINE
SF=500

mkdir -p $LOG_DIR/ssb/$SF/

numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q11 > $LOG_DIR/ssb/$SF/q11
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q12 > $LOG_DIR/ssb/$SF/q12
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q13 > $LOG_DIR/ssb/$SF/q13
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q21 > $LOG_DIR/ssb/$SF/q21
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q22 > $LOG_DIR/ssb/$SF/q22
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q23 > $LOG_DIR/ssb/$SF/q23
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q31 > $LOG_DIR/ssb/$SF/q31
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q32 > $LOG_DIR/ssb/$SF/q32
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q33 > $LOG_DIR/ssb/$SF/q33
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q34 > $LOG_DIR/ssb/$SF/q34
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q41 > $LOG_DIR/ssb/$SF/q41
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q42 > $LOG_DIR/ssb/$SF/q42
numactl --physcpubind=24-47 --membind=1 ../bin/ssb/q43 > $LOG_DIR/ssb/$SF/q43


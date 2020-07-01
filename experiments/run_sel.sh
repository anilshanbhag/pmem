#!/bin/bash

LOG_DIR=../logs
LOG1=$LOG_DIR/select
LOG2=$LOG_DIR/select_nvm

numactl --physcpubind=24-47 --membind=1 ../bin/select > $LOG1
numactl --physcpubind=24-47 --membind=1 ../bin/select --m=pmem >> $LOG2


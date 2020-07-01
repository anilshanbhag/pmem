#!/bin/bash

LOG_DIR=../logs
LOG1=$LOG_DIR/join
LOG2=$LOG_DIR/join_nvm

rm -f $LOG1
touch $LOG1
rm -f $LOG2
touch $LOG2

for i in `seq 10 31`;
do
 echo $i
 numactl --physcpubind=24-47 --membind=1 ../bin/join --n=268435456 --d=$((2 ** $i)) >> $LOG_DIR/join
 numactl --physcpubind=24-47 --membind=1 ../bin/join --m=pmem --n=268435456 --d=$((2 ** $i)) >> $LOG_DIR/join_nvm
done



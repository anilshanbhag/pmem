#!/bin/bash

LOG_DIR=../logs
LOG=$LOG_DIR/threads

rm -f $LOG
touch $LOG

echo "Running Threads"

for i in `seq 24 47`;
do
 echo $i
 numactl --physcpubind=24-$i --membind=1 ../bin/bandwidth >> $LOG
done

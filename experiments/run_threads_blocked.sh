#!/bin/bash

LOG_DIR=../logs
LOG=$LOG_DIR/threads_blocked.txt

rm -f $LOG
touch $LOG

echo "Running Threads"

for i in `seq 0 11`;
do
 echo $i
 numactl --physcpubind=0-$i --membind=0 ../bin/bandwidth --t=5 >> $LOG
done


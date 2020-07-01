#!/bin/bash

LOG_DIR=../logs/externaljoin

#for i in `seq 1 16`;
#do
 #echo $i;
 #numactl --physcpubind=24-47 --membind=1 ../bin/externaljoin/join1 --m=pmem --n=$((16 * 2**27)) --d=$((2**27 * $i)) > $LOG_DIR/join1_$i
#done

#for i in `seq 1 16`;
#do
 #echo $i;
 #numactl --physcpubind=24-47 --membind=1 ../bin/externaljoin/join2 --m=pmem --n=$((16 * 2**27)) --d=$((2**27 * $i)) > $LOG_DIR/join2_$i
#done

#for i in `seq 1 16`;
#do
 #echo $i;
 #numactl --physcpubind=24-47 --membind=1 ../bin/join --m=pmem --n=$((16 * 2**27)) --d=$((2**27 * $i)) > $LOG_DIR/join3_$i
#done

for i in `seq 1 16`;
do
 echo $i;
 numactl --physcpubind=24-47 --membind=1 ../bin/join --n=$((16 * 2**27)) --d=$((2**27 * $i)) > $LOG_DIR/join4_$i
done



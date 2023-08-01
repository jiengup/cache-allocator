#!/bin/bash

for x in {1..8}
do
    trace_file="2016020${x}00-2016020$((x+1))00.csv_sort.csv"
	../build/simulator \
	--name cloudphoto-day${x}-reply \
	-a lru \
	-f /data1/cloud_photo/$trace_file \
	--total-cache-size 1536g \
	--server-cache-sizes=256g,256g,256g,256g,256g,256g \
	-n 6 \
	-t cloudphoto \
	-l 60 \
	-o /data1/logs
done

filename="2016020900-2016021000.csv_sort.csv"
../build/simulator \
--name cloudphoto-day${x}-reply \
-a lru \
-f /data1/cloud_photo/$trace_file \
--total-cache-size 1536g \
--server-cache-sizes=256g,256g,256g,256g,256g,256g \
-n 6 \
-t cloudphoto \
-l 60 \
-o /data1/logs

../build/simulator \
--name cloudphoto-day2-reply \
-a lru \
-f /data1/cloud_photo/2016020100-2016020200.csv_sort.csv \
--total-cache-size 1536g \
--server-cache-sizes=256g,256g,256g,256g,256g,256g \
-n 6 \
-t cloudphoto \
-l 60 \
-o /data1/logs

# pushd ../build;
# make;
# popd;
# ../build/simulator \
# --name cloudphoto-day2-dp-solve \
# --task-type dp-solve \
# --input-file-path /root/cacheAllocation/result/flows_res_1000/2016020200-2016020300_unique \
# -n 6 \
# --traffic-file-path /root/cacheAllocation/result/traffic_stat/2016020200-2016020300_unique \
# -o /data1/


pushd ../build;
make;
popd;
../build/simulator \
--name cloudphoto-day2-dp-solve \
--task-type dp-solve \
--input-file-path /root/cacheAllocation/result/flows_res_1000/2016020200-2016020300_unique \
-n 6 \
--traffic-file-path /root/cacheAllocation/result/traffic_stat/2016020200-2016020300_unique \
-o /data1/
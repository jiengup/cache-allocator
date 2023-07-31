pushd ../build
make
popd
../build/simulator \
--name cache_beh \
-a lru \
-f ../dataset/cloud_photos-sample/test.csv \
--total-cache-size 1025 \
--server-cache-sizes=1025 \
-n 1 \
-t cloudphoto \
-l 1

pushd ../build
make
popd
../build/simulator \
--name test \
-a lru \
-f ../dataset/cloud_photos-sample/cloud_photos-sample.csv \
--total-cache-size 6g \
--server-cache-sizes=1g,1g,1g,1g,1g,1g \
-n 6 \
-t cloudphoto \
-l 1

# cache-allocator
A reuse-distance-profie based dynamic cache space allocator for multi-tenant CDN.

## build(get the binary `simulator`
```bash
mkdir build
cd build
cmake ..
make
```

## example
```
../build/simulator \
--name test \
-a lru \
-f ../dataset/cloud_photos-sample/cloud_photos-sample.csv \
--total-cache-size 6g \
--server-cache-sizes=1g,1g,1g,1g,1g,1g \
-n 6 \
-t cloudphoto \
-l 1
```

## suported function now
- replay of the csv trace
  - csv format: with header, \[timestamp, obj_id, tenent_id, obj_size\]
- TODO

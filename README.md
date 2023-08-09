# cache-allocator
A reuse-distance-profie based dynamic cache space allocator for multi-tenant CDN.

## build(get the binary `simulator`)
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

## suported task type
- replay: replay of the csv trace
  - csv format: with header, \[timestamp, obj_id, tenent_id, obj_size\]
- dp-solve: optimazation solver
  - optimization problem: 
  $$min(traffic_i*BMRC(ize_i)) \\
  subject.to. \sum_{i}size_i = TotalSize$$
## workflow
### 1. profile MRC
source code: `third_party/flows`

1.1 compile command (get binary `flows`):
```bash
cd third_party/flows
make
```
1.2 run profile for each tenant and get tenant-level MRC
script: `exp/scripts/[profiling method]_multi_tenant.py`

for modification:
- line 5: dataset path (e.g. /data1/tenant_trace **in cloudlab**)
```bash
guntherX@clnode146:/data1/tenant_trace$ ls
2016020100-2016020200_unique  2016020300-2016020400_unique  2016020500-2016020600_unique  2016020700-2016020800_unique  2016020900-2016021000_unique
2016020200-2016020300_unique  2016020400-2016020500_unique  2016020600-2016020700_unique  2016020800-2016020900_unique
guntherX@clnode146:/data1/tenant_trace$ ll -h 2016020300-2016020400_unique
total 40G
drwxr-xr-x  2 root root 4.0K Aug  5 05:20 ./
drwxr-xr-x 11 root root 4.0K Aug  5 06:27 ../
-rw-r--r--  1 root root 9.5K Aug  5 06:48 tenant_1_trace
-rw-r--r--  1 root root 2.2G Aug  5 06:48 tenant_2_trace
-rw-r--r--  1 root root  43K Aug  5 06:48 tenant_3_trace
-rw-r--r--  1 root root  23G Aug  5 06:48 tenant_4_trace
-rw-r--r--  1 root root 9.5G Aug  5 06:48 tenant_5_trace
-rw-r--r--  1 root root 5.6G Aug  5 06:48 tenant_6_trace
```
- line 11-13: profile parameters(e.g. profiling method, FIX-SAMPLE/FIX-RATE, sample rate, etc. **see ./flows --help for detail**)

- line 38: threads number to run

- line 6: output directory (e.g. /opt/flows_res_1000)
  - output filename format:
    - profile result: tenant_[tenant_id]_profile_res-[sample_method]-[profile_method]-[sample_rate].csv
    - profiling log: tenant_[tenant_id]_profile_res-[sample_method]-[profile_method]-[sample_rate].log: profiling log
  - profile result csv file header: \[cache_size, bmrc_ratio, omrc_ratio\]

### 2. interpolation
script: `misc/interpolation.py`

for modification:

line 6: profiling result dir for interpolation(e.g. result/flows_res_1000 from step 1)

output: interpolation result csv file (e.g. result/flows_res_1000/2016020100-2016020200_unique/tenant_1_profile_res-FIX_RATE-FLOWS-0.01.csv -> result/flows_res_1000/2016020100-2016020200_unique/tenant_1_profile_res-FIX_RATE-FLOWS-0.01_inter.csv)

### 3. get tenant-level traffic stat
script: `exp/scripts/collect_result.py`

for modification:
- line 4: input log file dir (e.g. result/flows_res_1000 from step 1)

- line 5: traffic_out_dir (e.g. result/traffic_stat)

output: tenant-level traffic stat csv file (e.g. result/traffic_stat/2016020100-2016020200_unique)

header: \[tenant_id, total_access, total_traffic, unique_access, unique_traffic\]


### 4. run solver
example script: `exp/cloudphoto-day2-dp-solve.sh`

### 5. run replay for evaluation
example script: `exp/cloudphoto-day2-replay.sh`

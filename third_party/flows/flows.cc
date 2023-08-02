/*
1.Demo需求：
命令行解析：
a.实现：FLOWS、SHARDS、Carra的方法
b.实现：MAE和MAEQ的指标，点数为100
c.实现：根据采样率和采样数的两种方法
d.实现：可以控制采样率和采样数
e.实现：输出真实MRC(MAEQ和MAE两个版本，点数为1000点)
f.实现：根据输入的trace进行测试
g.实现：输出结果到文件，统计trace的workingset
h.实现：输出程序的执行时间

编译：g++ flows -o flows -O3
帮助：./flows 

data sample:
0 0 1594 0
0 1 3421 0
0 2 16079 1
0 3 17012 0
0 4 11239 1
0 5 5506 0
0 6 36809 1
0 7 3876 0
0 8 52774 1
0 9 13413 1
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <fcntl.h>
#include <unistd.h>
#include <queue>
#include <set>
#include "cmdline.h"
#include "splaytree.h"
#include "LRU_fix_capacity.h"
#include "LRU_fix_count.h"
using namespace std;

#define TEST_POINT_NUM 100
uint64_t g_hash_mask = 0;
ofstream out_file;

uint64_t hash_uint64(uint64_t x, uint64_t seed) {
    x ^= seed;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53;
    x ^= x >> 33;
    x ^= seed;
    return x;
}

template<typename K, typename V>
class MinValueMap {
public:
    MinValueMap(size_t n) : n(n) {}

    K insert(const K& key, const V& value) {
        auto it = map.find(key);
        if (it != map.end()) {
            // Key already exists, update its value
            auto setIt = set.find({it->second, it->first});
            set.erase(setIt);
            set.insert({value, key});
            it->second = value;
        } else {
            // New key
            map[key] = value;
            if (set.size() < n) {
                set.insert({value, key});
            } else if (value < set.rbegin()->first) {
                auto last = *set.rbegin();
                set.erase(last);
                set.insert({value, key});
                map.erase(last.second);
                return last.second;
            }
        }
        return -1;
    }

    bool full() const {
        return set.size() == n;
    }

    bool empty() const {
        return set.empty();
    }

    V get_max_value() const {
        return set.rbegin()->first;
    }

private:
    size_t n;
    std::set<std::pair<V, K>> set;
    std::unordered_map<K, V> map;
};



class Trace{
public:
    Trace(string filename):filename(filename), now_idx(0),
    total_access_num(0),
    total_access_size(0),
    unique_access_num(0),
    unique_access_size(0),
    min_item_size(INT64_MAX),
    MAEQ_factor(0.0)
    {
        //读取文件中的所有请求
        FILE * fp = fopen(filename.c_str(), "r");
        if(fp == NULL){
            cout << "open file error" << endl;
            exit(1);
        }
        printf("open file %s\n", filename.c_str());

        char buf[1024];
        int64_t time_stamp, key, size, oth;
        ;
        while(fscanf(fp, "%ld %ld %ld %ld", &time_stamp, &key, &size, &oth) != EOF){
            // printf("%ld %ld %ld %ld\n", time_stamp, key, size, oth);
            key_size.push_back(make_pair(key, size));
            if(key_size.size() %1000000 ==0){
                // printf("read %ld lines\n", key_size.size());
            }
        }
        fclose(fp);

        get_working_set_size();
    }

    void reset(){
        now_idx = 0;
    }

    pair<int64_t, int64_t> get_next(){
        if(now_idx >= key_size.size()){
            return make_pair(-1, -1);
        }
        return key_size[now_idx++];
    }

    string filename;
    int64_t now_idx;
    int64_t total_access_num;
    int64_t total_access_size;
    int64_t unique_access_num;
    int64_t unique_access_size;
    int64_t min_item_size;
    double MAEQ_factor;
    vector<pair<int64_t, int64_t>> key_size;

    void get_working_set_size(){
        unordered_map<int64_t, int64_t> key_seen_time;
        total_access_num = 0;
        total_access_size = 0;
        unique_access_num = 0;
        unique_access_size = 0; 
        int64_t key, size;
        reset();
        
        while(true){
            pair<int64_t, int64_t> key_size = get_next();
            if(key_size.first == -1){
                break;
            }
            key = key_size.first;
            size = key_size.second;
            total_access_size += size;
            total_access_num ++;
            min_item_size = min(min_item_size, size);
            min_item_size = max((int64_t)1, min_item_size);
            if(key_seen_time.count(key) == 0){
                key_seen_time[key] = 1;
                unique_access_num ++;
                unique_access_size += size;
            }
            else{
                key_seen_time[key] ++;
            }
        }

        cout << "[FILE]: "<< filename << endl;
        cout << "total_access_num: " << total_access_num << endl;
        cout << "total_access_size: " << total_access_size << endl;
        cout << "unique_access_num: " << unique_access_num << endl;
        cout << "unique_access_size: " << unique_access_size << endl;
        cout << "min_item_size: " << min_item_size << endl;

        
        reset();
    }
    // 对于大于working_set_size的，直接记录在inf，理论上不应该有这样的现象
    int get_MAE_bin_idx(int64_t rd, int test_points){
        int bin_idx = rd * test_points / unique_access_size;
        bin_idx = min(bin_idx, test_points - 1);
        return bin_idx;
    }
    // 返回这个bin_idx最大的rd
    int64_t get_MAE_bin_size(int bin_idx, int test_points){
        int64_t bin_size = (bin_idx + 1) * unique_access_size / test_points;
        return bin_size;
    }

    // min_item_size * MAEQ_factor ^ i >= rd
    int get_MAEQ_bin_idx(int64_t rd, int test_points){
        int bin_idx = log(rd / min_item_size) / log(MAEQ_factor);
        bin_idx = min(bin_idx, test_points - 1);
        bin_idx = max(bin_idx, 0);
        return bin_idx;
    }

    int64_t get_MAEQ_bin_size(int bin_idx, int test_points){
        int64_t bin_size = min_item_size * pow(MAEQ_factor, bin_idx);
        return bin_size;
    }

    void get_MAEQ_factor(int test_points){
        // 计算MAEQ_factor
        MAEQ_factor = powf(1.0 * unique_access_size / min_item_size, 1.0 / (test_points-1));
    }
};


void carra_mrc_fix_rate(Trace &trace, string output, string metric, double sample_metric,int test_points = TEST_POINT_NUM){
    trace.reset();
    SplayTree<int64_t,int64_t> rd_tree;
    unordered_map<int64_t, int64_t> item_last_access_time;
    trace.get_MAEQ_factor(test_points);
    printf("MAEQ_factor: %lf\n", trace.MAEQ_factor);

    bool is_MAE = metric == "MAE";

    vector<int64_t> BMRC_hist(test_points, 0);
    vector<int64_t> OMRC_hist(test_points, 0);
    vector<double> BMRC_ratio(test_points, 0);
    vector<double> OMRC_ratio(test_points, 0);

    vector<int64_t> small_cache_BMRC_hist(test_points, 0);
    vector<int64_t> small_cache_OMRC_hist(test_points, 0);
    vector<double> small_cache_BMRC_ratio(test_points, 0);
    vector<double> small_cache_OMRC_ratio(test_points, 0);

    double sample_rate = sample_metric;
    
    int64_t key, size;
    int64_t bin_idx = 0;
    int64_t now_time = 0;
    uint64_t sample_max = UINT64_MAX * sample_rate;
    int64_t sampled_cnt = 0, sampled_size = 0;


    int64_t small_cache_size = trace.unique_access_size/trace.unique_access_num * log(1/sample_rate) *(1/sample_rate);
    LRU_fix_capacity<int64_t, int64_t> small_cache(small_cache_size);

    int64_t cnt = 0;
    while(true){
        // if(cnt % 1 == 0){
        //     printf("%ld\n", cnt);
        // }
        cnt ++;
        pair<int64_t, int64_t> key_size = trace.get_next();
        if(key_size.first == -1){
            break;
        }
        key = key_size.first;
        size = key_size.second;
        if(small_cache.find(key)){
            small_cache.set(key, size);
            int64_t rd = small_cache.getLastDistance();
            
            if(is_MAE){
                bin_idx = trace.get_MAE_bin_idx(rd, test_points);
            }
            else{
                bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
            }
            small_cache_BMRC_hist[bin_idx] += size;
            small_cache_OMRC_hist[bin_idx] += 1;
        }
        else{
            small_cache.set(key, size);
        }

        uint64_t hash_value = hash_uint64(key, g_hash_mask);
        if(hash_value <= sample_max){
            // sampled!
            now_time ++;
            sampled_cnt ++;
            sampled_size += size; 
            if(item_last_access_time.count(key)){
                // in reuse distance tree
                int64_t last_acc_time = item_last_access_time[key];
                int64_t rd = rd_tree.getDistance(last_acc_time)/sample_rate;

                item_last_access_time[key] = now_time;
                rd_tree.erase(last_acc_time);
                rd_tree.insert(now_time, size);
                if(is_MAE){
                    bin_idx = trace.get_MAE_bin_idx(rd, test_points);
                }
                else{
                    bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
                }
                BMRC_hist[bin_idx] += size;
                OMRC_hist[bin_idx] += 1;
            }
            else{
                // not in reuse distance tree
                item_last_access_time[key] = now_time;
                rd_tree.insert(now_time, size);
            }
        }
    }

    //调整直方图
    int64_t should_sampled_size = trace.total_access_size * sample_rate;
    int64_t should_sampled_cnt = trace.total_access_num * sample_rate;
    BMRC_hist[0] += should_sampled_size  - sampled_size;
    OMRC_hist[0] += should_sampled_cnt  - sampled_cnt;
    

    //计算累计直方图
    for(int i = 1; i < test_points; i++){
        BMRC_hist[i] += BMRC_hist[i-1];
        OMRC_hist[i] += OMRC_hist[i-1];
        small_cache_BMRC_hist[i] += small_cache_BMRC_hist[i-1];
        small_cache_OMRC_hist[i] += small_cache_OMRC_hist[i-1];
    }

    for(int i = 0; i < test_points; i++){
        BMRC_ratio[i] = min(1.0, 1.0 - (double)BMRC_hist[i] / should_sampled_size);
        OMRC_ratio[i] = min(1.0, 1.0 - (double)OMRC_hist[i] / should_sampled_cnt);


        small_cache_BMRC_ratio[i] = min(1.0, 1.0 - (double)small_cache_BMRC_hist[i] / trace.total_access_size);
        small_cache_OMRC_ratio[i] = min(1.0, 1.0 - (double)small_cache_OMRC_hist[i] / trace.total_access_num);
    }

    int small_cache_bin_idx = 0;
    if(is_MAE){
        small_cache_bin_idx = trace.get_MAE_bin_idx(small_cache_size + 1, test_points) - 1;
    }
    else{
        small_cache_bin_idx = trace.get_MAEQ_bin_idx(small_cache_size + 1, test_points) - 1;
    }

    if(small_cache_bin_idx >= 0){
        for(int i = 0; i < test_points; i++){
            if(i <= small_cache_bin_idx){
                BMRC_ratio[i] = small_cache_BMRC_ratio[i];
                OMRC_ratio[i] = small_cache_OMRC_ratio[i];
            }
            else{
                int64_t _now_cache_size = is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points);
                BMRC_ratio[i] = BMRC_ratio[i] + (small_cache_BMRC_ratio[small_cache_bin_idx] - BMRC_ratio[small_cache_bin_idx]) * exp(-(_now_cache_size - small_cache_size)/(4*small_cache_size));
                OMRC_ratio[i] = OMRC_ratio[i] + (small_cache_OMRC_ratio[small_cache_bin_idx] - OMRC_ratio[small_cache_bin_idx]) * exp(-(_now_cache_size - small_cache_size)/(4*small_cache_size));
            }
        }
    }

    printf("cache_size, BMRC_ratio, OMRC_ratio\n");
    for(int i = 0; i < test_points; i++){
        printf("%ld, %lf, %lf\n", is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points), BMRC_ratio[i], OMRC_ratio[i]);
    }
}


void flows_mrc_fix_rate(Trace &trace, string output, string metric, double sample_metric,int test_points = TEST_POINT_NUM, double slide_ratio = 0.5){
    trace.reset();
    SplayTree<int64_t,int64_t> rd_tree;
    unordered_map<int64_t, int64_t> item_last_access_time;
    trace.get_MAEQ_factor(test_points);
    printf("MAEQ_factor: %lf\n", trace.MAEQ_factor);

    bool is_MAE = metric == "MAE";

    vector<double> BMRC_hist(test_points, 0);
    vector<double> OMRC_hist(test_points, 0);
    vector<double> BMRC_ratio(test_points, 0);
    vector<double> OMRC_ratio(test_points, 0);

    double sample_rate = sample_metric;
    double weighted_sample_rate_base = sample_rate * (slide_ratio);
    double spatial_sample_rate = sample_rate * (1-slide_ratio);
    
    int64_t key, size;
    int64_t bin_idx = 0;
    int64_t rd = 0;
    int64_t now_time = 0;
    uint64_t sample_max = UINT64_MAX * spatial_sample_rate;
    int64_t s_avg = trace.unique_access_size/trace.unique_access_num;
    double sampled_cnt = 0, sampled_size = 0;


    int64_t small_cache_count = log(1/sample_rate) *(1/sample_rate);
    LRU_fix_count<int64_t, int64_t> small_cache(small_cache_count);

    while(true){
        now_time ++;
        pair<int64_t, int64_t> key_size = trace.get_next();
        if(key_size.first == -1){
            break;
        }
        key = key_size.first;
        size = key_size.second;
        if(small_cache.find(key)){
            // hit in cache filter
            small_cache.set(key, size);
            rd = small_cache.getLastDistance();
            if(is_MAE){
                bin_idx = trace.get_MAE_bin_idx(rd, test_points);
            }
            else{
                bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
            }
            BMRC_hist[bin_idx] += size;
            OMRC_hist[bin_idx] += 1;
            sampled_cnt += 1;
            sampled_size += size;
        }
        else{
            // miss in cache filter

            uint64_t hash_value = hash_uint64(key, g_hash_mask);
            double weighted_sample_rate = min(1.0, weighted_sample_rate_base * size/s_avg);
            uint64_t weighted_sample_max = weighted_sample_rate * UINT64_MAX;

            if(hash_value <= sample_max || hash_value <= weighted_sample_max){
                if(item_last_access_time.count(key)){
                    int64_t last_acc_time = item_last_access_time[key];
                    rd = rd_tree.getDistance(last_acc_time);
                    rd += small_cache.getNowSize();
                    if(is_MAE){
                        bin_idx = trace.get_MAE_bin_idx(rd, test_points);
                    }
                    else{
                        bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
                    }

                    if(hash_value <= sample_max){
                        OMRC_hist[bin_idx] += 1.0/spatial_sample_rate;
                    }
                    if(hash_value <= weighted_sample_max){
                        BMRC_hist[bin_idx] += size/weighted_sample_rate;
                        rd_tree.erase(last_acc_time);
                    }
                }
                
                if(hash_value <= sample_max){
                    sampled_cnt += 1.0/spatial_sample_rate;
                }
                if(hash_value <= weighted_sample_max){
                    sampled_size += size/weighted_sample_rate;
                }
            }

            small_cache.set(key,size);
            if(small_cache.length() > small_cache_count){
                //TODO: prob insert

                auto pop_k_size = small_cache.pop();
                int64_t pop_key = pop_k_size.first;
                int64_t pop_size = pop_k_size.second;

                uint64_t pop_hash_value = hash_uint64(pop_key, g_hash_mask);
                double pop_weighted_sample_rate = min(1.0, weighted_sample_rate_base * pop_size/s_avg);
                uint64_t pop_weighted_sample_max = pop_weighted_sample_rate * UINT64_MAX;

                if(pop_hash_value <= sample_max || pop_hash_value <= pop_weighted_sample_max){
                    item_last_access_time[pop_key] = now_time;
                    if(pop_hash_value <= pop_weighted_sample_max){
                        rd_tree.insert(now_time, pop_size/pop_weighted_sample_rate);
                    }
                }
            }
        }
    }
    // modify hist
    int64_t avg_small_cache_size = small_cache_count * s_avg;
    int start_bin_idx = 0;
    if(is_MAE){
        start_bin_idx = trace.get_MAE_bin_idx(avg_small_cache_size + 1, test_points);
    }
    else{
        start_bin_idx = trace.get_MAEQ_bin_idx(avg_small_cache_size + 1, test_points);
    }
    double remain_size = trace.total_access_size - sampled_size;
    double remain_cnt = trace.total_access_num - sampled_cnt;
    // printf("%lf, %ld\n", sampled_size, trace.total_access_size);
    // printf("%lf, %ld\n", sampled_cnt, trace.total_access_num);
    for(int i = start_bin_idx; i < test_points; i++){
        double add_size = 0;
        double add_cnt = 0;

        if(remain_size > 0){
            add_size = min(remain_size, BMRC_hist[i]);
        }
        else{
            add_size = max(remain_size, -0.5*BMRC_hist[i]);
        }
        if(remain_cnt > 0){
            add_cnt = min(remain_cnt, OMRC_hist[i]);
        }
        else{
            add_cnt = max(remain_cnt, -0.5*OMRC_hist[i]);
        }
        remain_size -= add_size;
        remain_cnt -= add_cnt;

        BMRC_hist[i] += add_size;
        OMRC_hist[i] += add_cnt;
    }

    

    //计算累计直方图
    for(int i = 1; i < test_points; i++){
        BMRC_hist[i] += BMRC_hist[i-1];
        OMRC_hist[i] += OMRC_hist[i-1];
    }

    for(int i = 0; i < test_points; i++){
        BMRC_ratio[i] = min(1.0, 1.0 - (double)BMRC_hist[i] / trace.total_access_size);
        OMRC_ratio[i] = min(1.0, 1.0 - (double)OMRC_hist[i] / trace.total_access_num);
    }


    printf("cache_size, BMRC_ratio, OMRC_ratio\n");
    for(int i = 0; i < test_points; i++){
        printf("%ld, %lf, %lf\n", is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points), BMRC_ratio[i], OMRC_ratio[i]);
    }
}



void flows_mrc_fix_cnt(Trace &trace, string output, string metric, double sample_metric,int test_points = TEST_POINT_NUM, double weighted_slide_ratio = 0.5){
    trace.reset();
    SplayTree<int64_t,int64_t> small_req_rd_tree;
    SplayTree<int64_t,int64_t> large_req_rd_tree;
    unordered_map<int64_t, int64_t> item_last_access_time;
    trace.get_MAEQ_factor(test_points);
    printf("MAEQ_factor: %lf\n", trace.MAEQ_factor);

    bool is_MAE = metric == "MAE";

    vector<double> BMRC_hist(test_points, 0);
    vector<double> OMRC_hist(test_points, 0);
    vector<double> BMRC_ratio(test_points, 0);
    vector<double> OMRC_ratio(test_points, 0);

    int sample_count = sample_metric;
    int64_t weighted_sample_cnt = sample_count * (weighted_slide_ratio);
    double spatial_sample_cnt = sample_count * (1-weighted_slide_ratio);
    MinValueMap<int64_t, double> weighted_sample_map(weighted_sample_cnt);
    MinValueMap<int64_t, uint64_t> spatial_sample_map(spatial_sample_cnt);
    double weighted_sample_rate = 1.0;
    double spatial_sample_rate = 1.0;
    
    int64_t key, size;
    int64_t bin_idx = 0;
    int64_t rd = 0;
    int64_t now_time = 0;
    double weighted_sample_max = UINT64_MAX;
    uint64_t spatial_sample_max = UINT64_MAX;
    double s_avg = trace.unique_access_size/trace.unique_access_num;
    double sampled_cnt = 0, sampled_size = 0;

    while(true){
        now_time ++;
        pair<int64_t, int64_t> key_size = trace.get_next();
        if(key_size.first == -1){
            break;
        }
        key = key_size.first;
        size = key_size.second;
        
        uint64_t hash_value = hash_uint64(key, g_hash_mask);
        double weighted_hash = ((1.0*hash_value/UINT64_MAX * s_avg/size));
        // double weighted_hash = min(1.0, ((1.0*hash_value)/(1.0*UINT64_MAX)));
        bool is_spatial_sampled = (!spatial_sample_map.full()) || hash_value <= spatial_sample_map.get_max_value();
        bool is_weighted_sampled = (!weighted_sample_map.full()) || weighted_hash <= weighted_sample_map.get_max_value();
        // double its_p = weighted_sample_rate * (size + 0.01)/s_avg;
        double its_p = min(1.0, weighted_sample_rate * (size + 0.01)/s_avg);

        if(is_spatial_sampled || is_weighted_sampled){
            ;
            if(is_spatial_sampled)
                spatial_sample_map.insert(key, hash_value);
                
            if(is_weighted_sampled){
                int64_t poped_key = weighted_sample_map.insert(key, weighted_hash);
                if(poped_key != -1){
                    int64_t poped_key_last_acc = item_last_access_time[poped_key];
                    large_req_rd_tree.erase(poped_key_last_acc);
                    small_req_rd_tree.erase(poped_key_last_acc);
                }
            }

            if(!spatial_sample_map.full()){
                spatial_sample_rate = 1.0;
                weighted_sample_rate = 1.0;
            }
            else{
                spatial_sample_rate = spatial_sample_map.get_max_value() * 1.0/UINT64_MAX;
                weighted_sample_rate = weighted_sample_map.get_max_value();
            }
            if(item_last_access_time.count(key)){
                // have seen
                // printf("%ld\n", key);
                int64_t last_acc_time = item_last_access_time[key];
                item_last_access_time[key] = now_time;
                int64_t small_req_rd = small_req_rd_tree.getDistance(last_acc_time) * s_avg/weighted_sample_rate;
                int64_t large_req_rd = large_req_rd_tree.getDistance(last_acc_time);
                rd = small_req_rd * 1.0 + large_req_rd;
                // assert(small_req_rd < trace.unique_access_size);
                
                if(is_weighted_sampled){
                    if(its_p == 1.0){
                        ;
                    }else{
                        small_req_rd -= s_avg;
                        small_req_rd += size*weighted_sample_rate;
                    }
                }
                else{
                    rd += size;
                }


                if(is_MAE){
                    bin_idx = trace.get_MAE_bin_idx(rd, test_points);
                }
                else{
                    bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
                }

                if(is_spatial_sampled){
                    // spatial sample
                    if(bin_idx < test_points){
                        OMRC_hist[bin_idx] += 1.0/spatial_sample_rate;
                    }
                    sampled_cnt += 1.0/spatial_sample_rate;
                }

                if(is_weighted_sampled){
                    if(bin_idx < test_points){
                        BMRC_hist[bin_idx] += 1.0*size/its_p;
                    }
                    sampled_size += 1.0*size/its_p;

                    if(its_p == 1.0){
                        //说明是大对象
                        large_req_rd_tree.erase(last_acc_time);
                        small_req_rd_tree.erase(last_acc_time);
                        large_req_rd_tree.insert(item_last_access_time[key], size);
                    }
                    else{
                        //说明当前的对象成为了一个小对象
                        large_req_rd_tree.erase(last_acc_time);
                        small_req_rd_tree.erase(last_acc_time);
                        small_req_rd_tree.insert(item_last_access_time[key], 1);
                    }
                }
            }
            else{
                // not seen
                item_last_access_time[key] = now_time;
                if(is_spatial_sampled){
                    sampled_cnt += 1.0/spatial_sample_rate;
                }
                if(is_weighted_sampled){
                    sampled_size += 1.0*size/its_p;
                    if(its_p == 1.0){
                        //说明是大对象
                        large_req_rd_tree.insert(item_last_access_time[key], size);
                    }
                    else{
                        //说明当前的对象成为了一个小对象
                        small_req_rd_tree.insert(item_last_access_time[key], 1);
                    }
                }
            }
        }
    }
    //调整直方图
    int64_t should_sampled_size = trace.total_access_size;
    int64_t should_sampled_cnt = trace.total_access_num;
    // printf("should_sampled_size: %ld\n", should_sampled_size);
    // printf("sampled_size: %lf\n", sampled_size);
    // printf("should_sampled_cnt: %ld\n", should_sampled_cnt);
    // printf("sampled_cnt: %lf\n", sampled_cnt);
    // printf("spatial_sample_rate: %lf\n", spatial_sample_rate);
    // printf("weighted_sample_rate: %lf\n", weighted_sample_rate);

    BMRC_hist[0] += should_sampled_size  - sampled_size;
    OMRC_hist[0] += should_sampled_cnt  - sampled_cnt;
    

    //计算累计直方图
    for(int i = 1; i < test_points; i++){

        BMRC_hist[i] += BMRC_hist[i-1];
        OMRC_hist[i] += OMRC_hist[i-1];
    }

    for(int i = 0; i < test_points; i++){
        BMRC_ratio[i] = min(1.0, 1.0 - (double)BMRC_hist[i] / trace.total_access_size);
        OMRC_ratio[i] = min(1.0, 1.0 - (double)OMRC_hist[i] / trace.total_access_num);
    }


    printf("cache_size, BMRC_ratio, OMRC_ratio\n");
    for(int i = 0; i < test_points; i++){
        printf("%ld, %lf, %lf\n", is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points), BMRC_ratio[i], OMRC_ratio[i]);
    }
}





void shards_adj_mrc_fix_rate(Trace &trace, string output, string metric, double sample_metric,int test_points = TEST_POINT_NUM){
    trace.reset();
    SplayTree<int64_t,int64_t> rd_tree;
    unordered_map<int64_t, int64_t> item_last_access_time;
    trace.get_MAEQ_factor(test_points);
    printf("MAEQ_factor: %lf\n", trace.MAEQ_factor);

    bool is_MAE = metric == "MAE";

    vector<int64_t> BMRC_hist(test_points, 0);
    vector<int64_t> OMRC_hist(test_points, 0);
    vector<double> BMRC_ratio(test_points, 0);
    vector<double> OMRC_ratio(test_points, 0);
    double sample_rate = sample_metric;
    
    int64_t key, size;
    int64_t bin_idx = 0;
    int64_t now_time = 0;
    uint64_t sample_max = UINT64_MAX * sample_rate;
    int64_t sampled_cnt = 0, sampled_size = 0;
    while(true){
        pair<int64_t, int64_t> key_size = trace.get_next();
        if(key_size.first == -1){
            break;
        }
        key = key_size.first;
        size = key_size.second;
        uint64_t hash_value = hash_uint64(key, g_hash_mask);

        if(hash_value <= sample_max){
            // sampled!
            now_time ++;
            sampled_cnt ++;
            sampled_size += size; 
            if(item_last_access_time.count(key)){
                // in reuse distance tree
                int64_t last_acc_time = item_last_access_time[key];
                int64_t rd = rd_tree.getDistance(last_acc_time)/sample_rate;

                item_last_access_time[key] = now_time;
                rd_tree.erase(last_acc_time);
                rd_tree.insert(now_time, size);
                if(is_MAE){
                    bin_idx = trace.get_MAE_bin_idx(rd, test_points);
                }
                else{
                    bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
                }
                BMRC_hist[bin_idx] += size;
                OMRC_hist[bin_idx] += 1;
            }
            else{
                // not in reuse distance tree
                item_last_access_time[key] = now_time;
                rd_tree.insert(now_time, size);
            }
        }
    }

    //调整直方图
    int64_t should_sampled_size = trace.total_access_size * sample_rate;
    int64_t should_sampled_cnt = trace.total_access_num * sample_rate;
    BMRC_hist[0] += should_sampled_size  - sampled_size;
    OMRC_hist[0] += should_sampled_cnt  - sampled_cnt;
    

    //计算累计直方图
    for(int i = 1; i < test_points; i++){
        BMRC_hist[i] += BMRC_hist[i-1];
        OMRC_hist[i] += OMRC_hist[i-1];
    }

    for(int i = 0; i < test_points; i++){
        BMRC_ratio[i] = min(1.0, 1.0 - (double)BMRC_hist[i] / should_sampled_size);
        OMRC_ratio[i] = min(1.0, 1.0 - (double)OMRC_hist[i] / should_sampled_cnt);
    }

    printf("cache_size, BMRC_ratio, OMRC_ratio\n");
    for(int i = 0; i < test_points; i++){
        printf("%ld, %lf, %lf\n", is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points), BMRC_ratio[i], OMRC_ratio[i]);
    }
}



void shards_adj_mrc_fix_cnt(Trace &trace, string output, string metric, double sample_metric,int test_points = TEST_POINT_NUM){
    trace.reset();
    SplayTree<int64_t,int64_t> rd_tree;
    unordered_map<int64_t, int64_t> item_last_access_time;
    trace.get_MAEQ_factor(test_points);
    printf("MAEQ_factor: %lf\n", trace.MAEQ_factor);

    bool is_MAE = metric == "MAE";

    vector<double> BMRC_hist(test_points, 0);
    vector<double> OMRC_hist(test_points, 0);
    vector<double> BMRC_ratio(test_points, 0);
    vector<double> OMRC_ratio(test_points, 0);
    int64_t sample_count = sample_metric;
    MinValueMap<int64_t, uint64_t> sample_map(sample_count);
    double sample_rate = 1.0;
    
    int64_t key, size;
    int64_t bin_idx = 0;
    int64_t now_time = 0;
    uint64_t sample_max = UINT64_MAX * sample_rate;
    double sampled_cnt = 0, sampled_size = 0;
    while(true){
        pair<int64_t, int64_t> key_size = trace.get_next();
        if(key_size.first == -1){
            break;
        }
        key = key_size.first;
        size = key_size.second;
        uint64_t hash_value = hash_uint64(key, g_hash_mask);

        if((!sample_map.full()) || hash_value <= sample_map.get_max_value()){
            // sampled!
            if(!sample_map.full()){
                sample_rate = 1.0;
            }
            else{
                sample_rate = sample_map.get_max_value() * 1.0/UINT64_MAX;
            }
            int64_t poped_key = sample_map.insert(key, hash_value);
            if(poped_key != -1){
                int64_t poped_key_last_acc = item_last_access_time[poped_key];
                rd_tree.erase(poped_key_last_acc);
            }

            now_time ++;
            sampled_cnt += 1.0/sample_rate;
            sampled_size += 1.0*size/sample_rate; 
            if(item_last_access_time.count(key)){
                // in reuse distance tree
                int64_t last_acc_time = item_last_access_time[key];
                int64_t rd = rd_tree.getDistance(last_acc_time);

                item_last_access_time[key] = now_time;
                rd_tree.erase(last_acc_time);
                rd_tree.insert(now_time, size/sample_rate);
                if(is_MAE){
                    bin_idx = trace.get_MAE_bin_idx(rd, test_points);
                }
                else{
                    bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
                }
                BMRC_hist[bin_idx] += 1.0*size/sample_rate;
                OMRC_hist[bin_idx] += 1.0/sample_rate;
            }
            else{
                // not in reuse distance tree
                item_last_access_time[key] = now_time;
                rd_tree.insert(now_time, size/sample_rate);
            }
        }
    }

    //调整直方图
    int64_t should_sampled_size = trace.total_access_size;
    int64_t should_sampled_cnt = trace.total_access_num;
    // printf("should_sampled_size: %ld\n", should_sampled_size);
    // printf("sampled_size: %lf\n", sampled_size);
    // printf("should_sampled_cnt: %ld\n", should_sampled_cnt);
    // printf("sampled_cnt: %lf\n", sampled_cnt);
    // printf("sample_rate: %lf\n", sample_rate);

    BMRC_hist[0] += should_sampled_size  - sampled_size;
    OMRC_hist[0] += should_sampled_cnt  - sampled_cnt;
    

    //计算累计直方图
    for(int i = 1; i < test_points; i++){
        BMRC_hist[i] += BMRC_hist[i-1];
        OMRC_hist[i] += OMRC_hist[i-1];
    }

    for(int i = 0; i < test_points; i++){
        BMRC_ratio[i] = min(1.0, 1.0 - (double)BMRC_hist[i] / should_sampled_size);
        OMRC_ratio[i] = min(1.0, 1.0 - (double)OMRC_hist[i] / should_sampled_cnt);
    }

    printf("cache_size, BMRC_ratio, OMRC_ratio\n");
    for(int i = 0; i < test_points; i++){
        printf("%ld, %lf, %lf\n", is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points), BMRC_ratio[i], OMRC_ratio[i]);
    }
}


void real_mrc(Trace &trace, string output, string metric, int test_points = TEST_POINT_NUM){
    trace.reset();
    SplayTree<int64_t,int64_t> rd_tree;
    unordered_map<int64_t, int64_t> item_last_access_time;
    trace.get_MAEQ_factor(test_points);
    printf("MAEQ_factor: %lf\n", trace.MAEQ_factor);

    bool is_MAE = metric == "MAE";

    vector<int64_t> BMRC_hist(test_points, 0);
    vector<int64_t> OMRC_hist(test_points, 0);
    vector<double> BMRC_ratio(test_points, 0);
    vector<double> OMRC_ratio(test_points, 0);
    
    int64_t key, size;
    int64_t bin_idx = 0;
    int64_t now_time = 0;
    while(true){
        now_time ++;
        pair<int64_t, int64_t> key_size = trace.get_next();
        if(key_size.first == -1){
            break;
        }
        key = key_size.first;
        size = key_size.second;
        if(item_last_access_time.count(key)){
            // in reuse distance tree
            int64_t last_acc_time = item_last_access_time[key];
            int64_t rd = rd_tree.getDistance(last_acc_time);
            if(now_time < 28){
                printf("%ld\n", now_time);
                printf("%ld\n", key);
                printf("%ld\n", size);
                printf("%ld\n", rd);
            }
            assert(rd >= size);

            item_last_access_time[key] = now_time;
            rd_tree.erase(last_acc_time);
            rd_tree.insert(now_time, size);
            if(is_MAE){
                bin_idx = trace.get_MAE_bin_idx(rd, test_points);
            }
            else{
                bin_idx = trace.get_MAEQ_bin_idx(rd, test_points);
            }
            BMRC_hist[bin_idx] += size;
            OMRC_hist[bin_idx] += 1;
        }
        else{
            // not in reuse distance tree
            item_last_access_time[key] = now_time;
            rd_tree.insert(now_time, size);
            // int64_t rd = INT64_MAX;
            // int64_t bin_idx = trace.get_MAE_bin_idx(rd, test_points);
        }
        
    }

    //计算累计直方图
    for(int i = 1; i < test_points; i++){
        BMRC_hist[i] += BMRC_hist[i-1];
        OMRC_hist[i] += OMRC_hist[i-1];
    }

    for(int i = 0; i < test_points; i++){
        BMRC_ratio[i] = min(1.0, 1.0 - (double)BMRC_hist[i] / trace.total_access_size);
        OMRC_ratio[i] = min(1.0, 1.0 - (double)OMRC_hist[i] / trace.total_access_num);
    }
    out_file << "cache_size, BMRC_ratio, OMRC_ratio" << endl;
    printf("cache_size, BMRC_ratio, OMRC_ratio\n");
    for(int i = 0; i < test_points; i++){
        out_file << (is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points)) << ", " << BMRC_ratio[i] << ", " << OMRC_ratio[i] << endl;
        printf("%ld, %lf, %lf\n", is_MAE ? trace.get_MAE_bin_size(i, test_points):trace.get_MAEQ_bin_size(i,test_points), BMRC_ratio[i], OMRC_ratio[i]);
    }
}

int main(const int argc, char * argv[]){
    cmdline::parser a;
    a.add<string>("trace", 't', "trace file name", true, "");
    a.add<string>("output", 'o', "output file name", true, "");
    a.add<string>("sample_method", 's', "FIX_RATE/FIX_NUM", false, "FIX_RATE");
    a.add<double>("sample_metric", 'r', "sample rate/ sample num", false, 0.01);
    a.add<string>("method", 'm', "REAL/SHARDS/CARRA/FLOWS", false, "REAL");
    a.add<string>("metric", 'c', "MAE/MAEQ", false, "MAE");
    a.add<uint64_t>("seed", 'd', "random seed", false, 42);
    
    a.parse_check(argc, argv);
    string tracefile = a.get<string>("trace");
    string output = a.get<string>("output");
    string sample_method = a.get<string>("sample_method");
    double sample_metric = a.get<double>("sample_metric");
    string method = a.get<string>("method");
    string metric = a.get<string>("metric");
    uint64_t seed = a.get<uint64_t>("seed");
    g_hash_mask = hash_uint64(seed, 0);

    out_file.open(output, ofstream::out | ofstream::trunc);
    printf("logging to %s", output.c_str());

    printf("tracefile: %s\n", tracefile.c_str());
    Trace trace(tracefile);

    auto time_start = chrono::steady_clock::now();
    cout <<"[method]: "<<method<< endl;
    cout <<"[metric]: "<<metric<< endl;
    cout <<"[seed]: "<<seed<< endl;
    cout <<"[sample_method]: "<<sample_method<< endl;
    cout <<"[sample_metric]: "<<sample_metric<< endl;
    if(method == "REAL"){
        trace.reset();
        real_mrc(trace, output,metric);
    }
    else if(method == "SHARDS"){
        if(sample_method == "FIX_RATE"){
            shards_adj_mrc_fix_rate(trace, output, metric, sample_metric);
        }
        else{
            shards_adj_mrc_fix_cnt(trace, output, metric, sample_metric);
        }
    }
    else if(method == "CARRA"){
        if(sample_method == "FIX_RATE"){
            carra_mrc_fix_rate(trace, output, metric, sample_metric);
        }
        else{
            ;
            assert(0);
        }
    }
    else if(method == "FLOWS"){
        if(sample_method == "FIX_RATE"){
            flows_mrc_fix_rate(trace, output, metric, sample_metric);
        }
        else{
            flows_mrc_fix_cnt(trace, output, metric, sample_metric);
        }
        
    }
    else{
        assert(0);
    }
    
    auto time_end = chrono::steady_clock::now();
    auto time_used = chrono::duration_cast<chrono::milliseconds>(time_end - time_start);
    cout << "Time used: " << time_used.count() << " ms" << endl;
    return 0;
}

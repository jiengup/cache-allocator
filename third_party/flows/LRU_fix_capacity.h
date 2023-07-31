#ifndef INCLUDE_LRU_FIX_CAPACITY_H
#define INCLUDE_LRU_FIX_CAPACITY_H

#include "splaytree.h"
#include <map>
#include <unordered_map>

template<typename K, typename V>
class LRU_fix_capacity {
public:
    LRU_fix_capacity(V cap) : capacity(cap), nowSize(0), 
    timeStamp(0){
        ;
    }

    V getLastDistance() {
        return lastDistance;
    }

    bool find(K key) {
        if(cache.count(key)) {
            return true;
        } else {
            return false;
        }
    }

    void set(K key, V size) {
        timeStamp ++;
        update(key, size, timeStamp);
    }

    // 将 Key 访问更新
    void update(K key, V size, int64_t timeStamp) {

        if(cache.count(key)){
            // key在cache中
            int64_t lastSeenTime = seenTime[key];
            // 更新重用距离
            lastDistance = myTree.getDistance(lastSeenTime);

            myTree.erase(lastSeenTime);
            myTree.insert(timeStamp, size);

            cache[key] = size;
            seenTime[key] = timeStamp;
            timeKeyMap.erase(lastSeenTime);
            timeKeyMap[timeStamp] = key;
        }
        else{
            // key不在cache中
            nowSize += size;
            cache[key] = size;
            seenTime[key] = timeStamp;
            timeKeyMap[timeStamp] = key;
            myTree.insert(timeStamp, size);

            lastDistance = INT64_MAX;

            if(nowSize > capacity){
                pop();
            }
        }
    }

    void pop() {
        if(nowSize <= capacity) return;
        while(nowSize > capacity){
            // printf("%ld, %ld, %ld\n", cache.size(),nowSize, capacity);
            auto t = *timeKeyMap.begin();
            int64_t popTimeStampe = t.first;
            K popKey = t.second;
            V size = cache[popKey];
            cache.erase(popKey);
            seenTime.erase(popKey);
            timeKeyMap.erase(popTimeStampe);
            myTree.erase(popTimeStampe);
            nowSize -= size;
        }
    }
    
private:
    V capacity;       // 容量
    int64_t timeStamp = 0;      // 时间戳
    V lastDistance;   // 访问距离
    V nowSize;        // 当前 Cache 大小
    
    std::unordered_map<K, int64_t> seenTime;
    std::unordered_map<K, V> cache;
    std::map<int64_t, K> timeKeyMap;

    SplayTree<int64_t, V> myTree;
};



#endif
#ifndef CACHECLUSTER_HPP
#define CACHECLUSTER_HPP

#include <libCacheSim.h>
#include "stat.hpp"
#include "cache_server.hpp"
#include <string>
#include <vector>

namespace CacheAllocation
{
    class CacheCluster
    {
    public:
        unsigned long n_server;
        unsigned long cluster_id;
        std::string cluster_name;
        std::string trace_file_path;
        std::string cache_algo;
        std::vector<CacheServer *> cache_servers;
        CacheClusterStat *cluster_stat;

        CacheCluster(const unsigned long cluster_id,
                     std::string cluster_name,
                     std::string trace_file_path,
                     CacheServer **cache_servers, const unsigned long n_server,
                     std::string cache_algo,
                     const unsigned long *server_cache_sizes,
                     unsigned int random_seed);

        void get(request_t *req);
        inline std::string get_cluster_stat_header()
        {
            std::string header = cluster_stat->stat_str_header() + ", used_cache_size/total_cache_size, ";
            for (unsigned long i=0; i<n_server; i++) {
                header += cache_servers[i]->server_stat->stat_str_header() + ", used_cache_size/total_cache_size, ";
            }
            header.pop_back();
            header.pop_back();
            return header;
        }

        inline std::string get_cluster_stat()
        {
            std::string stat = cluster_stat->stat_str() + get_cache_size_str() + ", ";
            for (unsigned long i=0; i<n_server; i++) {
                stat += cache_servers[i]->server_stat->stat_str() + ", " + cache_servers[i]->get_cache_size_str() + ", ";
            }
            stat.pop_back();
            stat.pop_back();
            return stat;
        }

        inline std::string get_cache_size_str()
        {
            std::stringstream ss;
            uint64_t cluster_used_cache_size = 0;
            unsigned long cluster_cache_size = 0;
            for (unsigned long i=0; i<n_server; i++) {
                cluster_cache_size += cache_servers[i]->cache_size;
                cluster_used_cache_size += cache_servers[i]->cache->get_occupied_byte(cache_servers[i]->cache);
            }
            ss << cluster_used_cache_size << "/" << cluster_cache_size;
            return ss.str();
        }
        ~CacheCluster();
    };
}

#endif /* CACHECLUSTER_HPP */
#include "cache_cluster.hpp"

namespace CacheAllocation
{
    CacheCluster::CacheCluster(const unsigned long cluster_id,
                               std::string cluster_name,
                               std::string trace_file_path,
                               CacheServer **cache_servers, const unsigned long n_server,
                               std::string cache_algo,
                               const unsigned long *cache_sizes,
                               unsigned int random_seed)
        : cluster_id(cluster_id), cluster_name(std::move(cluster_name)),
          trace_file_path(std::move(trace_file_path)), n_server(n_server),
          cache_algo(std::move(cache_algo))
    {
        cluster_stat = new CacheClusterStat(cluster_id);
        for (unsigned long i = 0; i < n_server; i++)
        {
            this->cache_servers.push_back(cache_servers[i]);
        }
        srand(random_seed);
    }
    void CacheCluster::get(request_t *req)
    {
        if (req->tenant_id == 0) 
        {
            WARN("request_tenant_id is 0, this is not allowed");
        }
        if (cache_servers[req->tenant_id-1]->get(req))
        {
            cluster_stat->hit_record(req);
        }
        else
        {
            cluster_stat->miss_record(req);
        }
    }
    CacheCluster::~CacheCluster()
    {
        delete cluster_stat;
    }
}
#ifndef STAT_HPP
#define STAT_HPP

#include "libCacheSim.h"
#include <string>
#include <sstream>
#include <unordered_set>

namespace CacheAllocation
{

  class CacheServerStat
  {
  public:
    unsigned long server_id;

    unsigned long req_cnt = 0;
    unsigned long hit_cnt = 0;
    unsigned long long req_bytes = 0;
    unsigned long long hit_bytes = 0;

    std::unordered_set<uint64_t> *obj_cnt;
    unsigned long long working_set_bytes = 0;

    CacheServerStat(const unsigned long server_id)
        : server_id(server_id), req_cnt(0), hit_cnt(0)
    {
      obj_cnt = new std::unordered_set<uint64_t>();
    };

    static std::string stat_str_header()
    {
      return "#server_id, hit_cnt/req_cnt, hit_bytes/req_bytes, "
             "working_set_size/working_set_bytes, "
             "omr/bmr";
    }
    std::string stat_str()
    {
      std::stringstream ss;
      ss.precision(6);
      ss << server_id << ", \t" << hit_cnt << "/" << req_cnt << ", \t"
         << hit_bytes << "/" << req_bytes << ", \t"
         << obj_cnt->size() << "/" << working_set_bytes << ", \t"
         << (double)hit_cnt / (double)req_cnt << "/" << (double)hit_bytes / (double)req_bytes << ", \t";
      return ss.str();
    }

    CacheServerStat(const CacheServerStat &stat) = delete;

    ~CacheServerStat() = default;

    CacheServerStat &operator=(const CacheServerStat &stat)
    {
      if (this == &stat)
        return *this;
      server_id = stat.server_id;
      req_cnt = stat.req_cnt;
      hit_cnt = stat.hit_cnt;

      return *this;
    }

    inline void hit_record(const request_t *req)
    {
      if (obj_cnt->count(req->obj_id) == 0)
      {
        obj_cnt->insert(req->obj_id);
        working_set_bytes += req->obj_size;
      }
      hit_cnt++;
      hit_bytes += req->obj_size;
      req_cnt++;
      req_bytes += req->obj_size;
    }

    inline void miss_record(const request_t *req)
    {
      if (obj_cnt->count(req->obj_id) == 0)
      {
        obj_cnt->insert(req->obj_id);
        working_set_bytes += req->obj_size;
      }
      req_cnt++;
      req_bytes += req->obj_size;
    }
  };

  class CacheClusterStat
  {
  public:
    unsigned long cluster_id;

    unsigned long cluster_req_cnt = 0;
    unsigned long cluster_hit_cnt = 0;
    unsigned long long cluster_req_bytes = 0;
    unsigned long long cluster_hit_bytes = 0;

    std::unordered_set<uint64_t> *obj_cnt;
    unsigned long long working_set_bytes = 0;

    CacheClusterStat(const unsigned long cluster_id)
        : cluster_id(cluster_id)
    {
      obj_cnt = new std::unordered_set<uint64_t>();
    }

    CacheClusterStat(const CacheClusterStat &stat) = delete;
    CacheClusterStat &operator=(const CacheClusterStat &stat) = delete;

    ~CacheClusterStat()
    {
      delete obj_cnt;
    }

    static std::string stat_str_header(bool with_server = false)
    {
      return "#cluster_id, hit_cnt/req_cnt, hit_bytes/req_bytes, "
             "working_set_size/working_set_bytes, "
             "omr/bmr";
    }

    std::string stat_str()
    {
      std::stringstream ss;
      ss.precision(6);
      ss << cluster_id << ", \t" << cluster_hit_cnt << "/" << cluster_req_cnt << ", \t"
         << cluster_hit_bytes << "/" << cluster_req_bytes << ", \t"
         << obj_cnt->size() << "/" << working_set_bytes << ", \t"
         << (double)cluster_hit_cnt / (double)cluster_req_cnt << "/" << (double)cluster_hit_bytes / (double)cluster_req_bytes << ", \t";
      return ss.str();
    }

    inline void hit_record(const request_t *req)
    {
      if (obj_cnt->count(req->obj_id) == 0)
      {
        obj_cnt->insert(req->obj_id);
        working_set_bytes += req->obj_size;
      }
      cluster_hit_cnt++;
      cluster_hit_bytes += req->obj_size;
      cluster_req_cnt++;
      cluster_req_bytes += req->obj_size;
    }

    inline void miss_record(const request_t *req)
    {
      if (obj_cnt->count(req->obj_id) == 0)
      {
        obj_cnt->insert(req->obj_id);
        working_set_bytes += req->obj_size;
      }
      cluster_req_cnt++;
      cluster_req_bytes += req->obj_size;
    }
  };
} // namespace CacheAllocation

#endif /* STAT_HPP */
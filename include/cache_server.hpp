#ifndef CACHESERVER_HPP
#define CACHESERVER_HPP

#include <libCacheSim.h>
#include <string>
#include "stat.hpp"
#include "const.hpp"

namespace CacheAllocation {

class CacheServer {
 public:
  cache_t *cache;
  std::string server_name;
  std::string exp_name;
  unsigned long server_id;
  // TODO: should cache size be represented in bytes?
  unsigned long cache_size;
  
  CacheServerStat *server_stat;
  
#ifdef TRACK_BYTE_REUSE
// ... variable define use for byte reuse
#endif

  CacheServer(unsigned long server_id, unsigned long cache_size,
              std::string cache_algo, 
              std::string exp_name = "default-name exp",
              std::string server_name = "default-name server",
              unsigned int random_seed = DEFAULT_RANDOM_SEED);

  inline bool get(const request_t *req) {
    if (this->cache->get(this->cache, req))
    {
      server_stat->hit_record(req);
      return true;
    }
    else
    {
      server_stat->miss_record(req);
      return false;
    }
  }

  inline std::string get_cache_size_str()
  {
    std::stringstream ss;
    ss << this->cache->get_occupied_byte(this->cache) << "/" << cache_size;
    return ss.str();
  }

  ~CacheServer();
};

}  // namespace CacheAllocation

#endif /* CACHESERVER_HPP */
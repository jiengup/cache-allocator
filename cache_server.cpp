#include "cache_server.hpp"

namespace CacheAllocation
{

  CacheServer::CacheServer(unsigned long server_id,
                           unsigned long cache_size,
                           std::string cache_algo,
                           std::string exp_name,
                           std::string server_name,
                           unsigned int random_seed)
      : exp_name(exp_name)
  {
    this->server_id = server_id;
    this->cache_size = cache_size;
    this->server_name = std::move(server_name);

    this->server_stat = new CacheServerStat(server_id);

    // cache replacement algorithm
    if (cache_algo == "lru")
    {
      common_cache_params_t cc_params = {
        .cache_size = cache_size, .consider_obj_metadata = false
      };
      this->cache = LRU_init(cc_params, NULL);
    } else {
      ERROR("unkonwn cache replacement algorithm %s\n", cache_algo.c_str());
      abort();
    }
    srand(random_seed);
  }
  CacheServer::~CacheServer()
  {
    delete this->server_stat;
    this->cache->cache_free(this->cache);
  }
}
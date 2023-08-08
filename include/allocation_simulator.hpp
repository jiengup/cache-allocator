#ifndef ALLOCATION_SIMULATOR_HPP
#define ALLOCATION_SIMULATOR_HPP

#include <string>
#include <libCacheSim.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "cxxopts.hpp"
#include "cache_server.hpp"
#include "cache_cluster.hpp"
#include "const.hpp"
#include "args.hpp"

namespace CacheAllocation
{
  class ExpRunner
  {
    allocation_arg_t *args;

    CacheCluster *cache_cluster;
    reader_t *reader;
    
    std::string ofilename;
    std::ofstream log_ofstream;

    unsigned long cache_start_time = 0;
    unsigned long last_log_time;
    unsigned long vtime;

    public:
      ExpRunner(allocation_arg_t *args);
      ~ExpRunner();
      
      void run_replay();
  };
} // namespace CacheAllocation


#endif // ALLOCATION_SIMULATOR_HPP

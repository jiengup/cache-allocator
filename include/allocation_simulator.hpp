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

namespace CacheAllocation
{
  typedef struct
  {
    std::string param_string;
    std::string trace_file_path;
    std::string cache_algo;
    std::string trace_type;
    std::string exp_name;

    trace_type_e reader_type;

    reader_init_param_t reader_init_params;
    unsigned long n_server;
    unsigned long server_cache_size;
    unsigned long *server_cache_sizes;
    std::string log_folder;
    unsigned long log_interval;
  } allocation_arg_t;

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

long convert_size(std::string str_size)
{
  if (std::isalpha(str_size.back()))
  {
    if (std::tolower(str_size.back()) == 'm')
    {
      str_size.pop_back();
      return std::stol(str_size) * 1024 * 1024;
    }
    else if (std::tolower(str_size.back()) == 'g')
    {
      str_size.pop_back();
      return std::stol(str_size) * 1024 * 1024 * 1024;
    }
    else if (std::tolower(str_size.back()) == 't')
    {
      str_size.pop_back();
      return std::stol(str_size) * 1024 * 1024 * 1024 * 1024;
    }
    else
    {
      str_size.pop_back();
      return std::stol(str_size);
    }
  }
  else
  {
    return std::stol(str_size);
  }
}

std::vector<std::string> mysplit(const std::string& str, const std::string& delimiter) {
  std::vector<std::string> tokens;
  std::string str_copy = str; // make a copy of the string
  size_t pos = 0;
  std::string token;
  while ((pos = str_copy.find(delimiter)) != std::string::npos) {
    token = str_copy.substr(0, pos);
    tokens.push_back(token);
    str_copy.erase(0, pos + delimiter.length());
  }
  tokens.push_back(str_copy);
  return tokens;
}

bool parse_cmd_arg(int argc, char *argv[],
                   CacheAllocation::allocation_arg_t *sargs)
{

  const time_t t = time(0);
  tm *ltm = localtime(&t);

  sargs->log_interval = 300;
  sargs->log_folder = "log_" + std::to_string(ltm->tm_mon + 1) + "_" +
                      std::to_string(ltm->tm_mday);
  mkdir(sargs->log_folder.c_str(), 0770);
  try
  {
    std::unique_ptr<cxxopts::Options> allocated(new cxxopts::Options(argv[0], "Run Cache Allocation experiment"));
    auto& options = *allocated;
    options
      .positional_help("[optional args]")
      .show_positional_help();

    std::string trace_type;
    std::string server_cache_size;

    options
      .set_width(70)
      .set_tab_expansion()
      .allow_unrecognised_options()
      .add_options()
      ("h,help", "print help")
      ("e,name", "experiment name", cxxopts::value<std::string>(sargs->exp_name)->default_value("allocation"))
      ("a,alg", "cache replacment algorithm", cxxopts::value<std::string>(sargs->cache_algo)->default_value("lru"))
      ("f,trace-path", "trace file path", cxxopts::value<std::string>(sargs->trace_file_path))
      ("total-cache-size", "per cache size(use avg)", cxxopts::value<std::string>(server_cache_size))
      ("server-cache-sizes", "list of cache sizes", cxxopts::value<std::vector<std::string>>())
      ("n,servers", "number of cache servers", cxxopts::value<unsigned long>(sargs->n_server))
      ("t,trace-type", "the type of trace[akamai1b/cloudphoto]", cxxopts::value<std::string>(sargs->trace_type))
      ("l,log-internal", "the log output interval in virtual time", cxxopts::value<unsigned long>(sargs->log_interval))
    ;

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << "here" << std::endl;
      std::cout << options.help({""}) << std::endl;
      return false;
    }

    sargs->server_cache_size = convert_size(server_cache_size);
    sargs->server_cache_sizes = new unsigned long[sargs->n_server];

    if (result.count("server-cache-sizes")) {
      auto &scs = result["server-cache-sizes"].as<std::vector<std::string>>();
      long sum_cache_size = 0;
      for (unsigned long i = 0; i < sargs->n_server; i++)
      {
        sum_cache_size += convert_size(scs.at(i));
        sargs->server_cache_sizes[i] = convert_size(scs.at(i));
      }
      if (std::abs(sum_cache_size - (long)(sargs->server_cache_size)) > 1024)
      {
        std::cerr << "sum of given cache sizes " << sum_cache_size / 1024 / 1024 / 1024 << "GB (" << sum_cache_size
                  << ")"
                  << " is not the same as avg cache size " << sargs->server_cache_size / 1024 / 1024 / 1024
                  << "GB - " << server_cache_size << "* n_server (" << sargs->n_server << ")" << std::endl;
        exit(0);
      }
    } else
    {
      INFO("fill all servers with same cache size %ld GB\n", sargs->server_cache_size / 1024 / 1024 / 1024);
      std::fill_n(sargs->server_cache_sizes, sargs->n_server,
                  sargs->server_cache_size);
    }
    std::vector<std::string> path_token = mysplit(sargs->trace_file_path, "/");
    sargs->exp_name = sargs->exp_name + "_" +
                      path_token[path_token.size() - 1] + "_" +
                      sargs->cache_algo + "_" +
                      "n_server_" + std::to_string(sargs->n_server) + ":";
    for (unsigned long i = 0; i < sargs->n_server; i++)
    {
      sargs->exp_name += std::to_string(sargs->server_cache_sizes[i] / 1024) + "MB_";
    }
    sargs->exp_name.pop_back();
    INFO("exp_name: %s\n", sargs->exp_name.c_str());
  }

  catch (const cxxopts::exceptions::exception& e)
  {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return false;
  }

  if (sargs->trace_type == "akamai1b")
  {
    // TODO: support akamai trace
    ERROR("not implemented yet\n");
    abort();
  }
  else if (sargs->trace_type == "cloudphoto")
  {
    sargs->reader_type = CSV_TRACE;
    reader_init_param_t init_params_csv = {
        .time_field = 1,
        .obj_id_field = 2,
        .obj_size_field = 4,
        .tenant_id_field = 3,
        .use_origin_key = true,
        .has_header = true,
        .delimiter = ',',
    };
    sargs->reader_init_params = init_params_csv;
  }
  else
  {
    ERROR("unknown trace type %s\n", optarg);
    abort();
  }
  INFO("Simulating %s\n", sargs->exp_name.c_str());
  INFO("trace %s, trace_type %s\n", sargs->trace_file_path.c_str(),
       sargs->trace_type.c_str());
  INFO("cache alg %s\n", sargs->cache_algo.c_str());
  INFO("n_server %lu\n", sargs->n_server);
  for (unsigned long i = 0; i < sargs->n_server; i++)
  {
    INFO("server %lu cache size %lu KB\n", i, sargs->server_cache_sizes[i]);
  }
  return true;
}
#endif // ALLOCATION_SIMULATOR_HPP
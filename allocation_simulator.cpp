#include "allocation_simulator.hpp"
#include "cxxopts.hpp"

namespace CacheAllocation
{
    ExpRunner::ExpRunner(allocation_arg_t *args): args(args), log_ofstream(), last_log_time(0), vtime(0) 
    {
        INFO(args->param_string.c_str());
        ofilename = args->log_folder + "/" + args->exp_name + ".vlog";
        log_ofstream.open(ofilename, std::ofstream::out | std::ofstream::trunc);

        this->reader = open_trace(args->input_file_path.c_str(),
                                      args->reader_type, 
                                      &args->reader_init_params);
        
        CacheServer *cache_servers[args->n_server];

        try {
            for (unsigned long i = 0; i<args->n_server; i++) {
                cache_servers[i] = new CacheServer(i, 
                args->server_cache_sizes[i], 
                args->cache_algo,
                args->exp_name,
                "server_" + std::to_string(i),
                DEFAULT_RANDOM_SEED);
            }
            
            this->cache_cluster = 
                new CacheCluster(1, 
                                 args->exp_name, 
                                 args->input_file_path, 
                                 cache_servers, 
                                 args->n_server, 
                                 args->cache_algo, 
                                 args->server_cache_sizes, 
                                 DEFAULT_RANDOM_SEED);

        } catch (std::exception &e) {
            std::cerr << "exception: " << e.what() << std::endl;
            print_stack_trace();
        }
    }
    ExpRunner::~ExpRunner()
    {
        log_ofstream.close();
        for (unsigned long i = 0; i<args->n_server; i++) {
            delete this->cache_cluster->cache_servers[i];
        }
        delete this->cache_cluster;
    }

    void ExpRunner::run_replay() {
        log_ofstream << "#vtime, " << this->cache_cluster->get_cluster_stat_header() << std::endl;
        request_t *req = new_request();
        while (read_one_req(this->reader, req) == 0) 
        {
            // print_request(req);
            this->cache_cluster->get(req);
            if (last_log_time == 0 || req->clock_time - last_log_time >= args->log_interval)
            {
                last_log_time = req->clock_time;
                log_ofstream << req->clock_time << ", " << this->cache_cluster->get_cluster_stat() << std::endl;
            }
            vtime ++;
        }
        log_ofstream << req->clock_time << ", " << this->cache_cluster->get_cluster_stat() << std::endl;
    }
}

void run_exp(int argc, char *argv[])
{
    CacheAllocation::allocation_arg_t args;
    bool ok = parse_cmd_arg(argc, argv, &args);
    for (int i = 1; i < argc; i++)
    {
        args.param_string += std::string(argv[i]) + " ";
    }
    args.param_string.pop_back();
    args.param_string += "\n";
    if (!ok)
    {
        ERROR("parse command parameters failed!");
        abort();
    }
    if (args.task_type == "reply")
    {
        CacheAllocation::ExpRunner *exp_runner = new CacheAllocation::ExpRunner(&args);
        exp_runner->run_replay();
    }
    else if (args.task_type == "dp-solve")
    {
        
    }
}

int main(int argc, char *argv[])
{
    try
    {
        run_exp(argc, argv);
    }
    catch (std::exception &e)
    {
        std::cerr << "exception: " << e.what() << std::endl;
        print_stack_trace();
        return 1;
    }
    return 0;
}

#ifndef DP_SOLVER_HPP
#define DP_SOLVER_HPP

#include <string>
#include <libCacheSim.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <regex>
#include "args.hpp"
#include "const.hpp"

namespace CacheAllocation
{
    typedef struct
    {
        uint64_t cache_size;
        uint32_t cache_size_in_gb;
        double_t bmrc_ratio;
        double_t omrc_ratio;
    } dp_entry_t;

    typedef struct
    {
        uint64_t total_request;
        uint64_t total_traffic;
        uint64_t unique_access;
        uint64_t unique_traffic;
    } traffic_stat_t;

    class DPSolver
    {
        allocation_arg_t *args;

        unsigned long n_server;
        uint32_t allocation_space_in_gb;
        std::vector<std::vector<dp_entry_t> > dp_table;
        std::vector<traffic_stat_t> traffic_stats;

        std::string ofilename;
        std::ofstream log_ofstream;
        public:
        DPSolver(allocation_arg_t *args);
        ~DPSolver();
        std::vector<uint32_t> optimal_solution;

        void print_dp_table();
        void print_stat_table();
        void print_optimal_solution();
        void solve();

        private:
        uint64_t _cost_cal(std::vector<uint32_t> solution);
        void read_res_csv_files();
        bool _read_res_csv_file(std::string filepath, int tenant_id);
        void read_stat_csv_file();

        void _brute_force_solve(int depth, std::vector<uint32_t> &solution, uint64_t &optimal_cost);
        void _greedy_search_solve(int depth, std::vector<uint32_t> &solution, uint64_t &optimal_cost);
    };
}

#endif // DP_SOLVER_HPP
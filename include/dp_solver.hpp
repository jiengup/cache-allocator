#ifndef DP_SOLVER_HPP
#define DP_SOLVER_HPP

#include <string>
#include <libCacheSim.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <experimental/filesystem>
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
        double omrc_ratio;
    } dp_entry_t;

    class DPSolver
    {
        allocation_arg_t *args;

        unsigned long n_server;
        std::vector<std::vector<dp_entry_t> > dp_table;

        std::string ofilename;
        std::ofstream log_ofstream;
        public:
        DPSolver(allocation_arg_t *args);
        ~DPSolver();

        private:
        void read_csv_files();
        bool _read_csv_file(std::string filepath, int tenant_id);
    };
}

#endif // DP_SOLVER_HPP
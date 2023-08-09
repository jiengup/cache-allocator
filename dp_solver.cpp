#include "dp_solver.hpp"

namespace CacheAllocation
{
    DPSolver::DPSolver(allocation_arg_t *args)
    {
        this->args = args;

        this->n_server = args->n_server;

        this->ofilename = args->log_folder + "/" + args->exp_name + ".log";
        this->log_ofstream.open(this->ofilename, std::ofstream::out | std::ofstream::trunc);
        // this->log_ofstream << "cache_size,cache_size_in_gb,bmrc_ratio,omrc_ratio\n";
        
        this->dp_table = std::vector<std::vector<dp_entry_t> >(this->n_server, std::vector<dp_entry_t>(0));
        this->traffic_stats = std::vector<traffic_stat_t>(this->n_server);

        read_res_csv_files();
        this->allocation_space_in_gb = this->dp_table[0].size();
        read_stat_csv_file();


    }

    DPSolver::~DPSolver()
    {
        this->log_ofstream.close();
    }

    void DPSolver::print_dp_table()
    {
        for (int i=0; i<this->dp_table[0].size(); i++) 
        {
            for (unsigned long j=0; j<this->n_server; j++)
            {
                std::cout << this->dp_table[j][i].cache_size_in_gb << " " << this->dp_table[j][i].bmrc_ratio << "\t";
            }
            std::cout << std::endl;
        }
    }

    void DPSolver::print_stat_table()
    {
        for (int i=0; i<this->n_server; i++) 
        {
            std::cout << i << " " 
                      << this->traffic_stats[i].total_request << " " 
                      << this->traffic_stats[i].total_traffic << " " 
                      << this->traffic_stats[i].unique_access << " " 
                      << this->traffic_stats[i].unique_traffic
                      << std::endl;
        }
    }

    void DPSolver::print_optimal_solution()
    {
        for (int i=0; i<this->n_server; i++)
        {
            std::cout << this->optimal_solution[i] << " ";
        }
        std::cout << std::endl;
    }

    void DPSolver::read_res_csv_files()
    {
        namespace fs  = std::filesystem;
        
        std::regex inter_csv_regex("inter\\.csv$");
        for (int cur_tenant_id = 1; cur_tenant_id <= (int)this->n_server; cur_tenant_id++) 
        {
            bool is_read = false;
            for (const auto& entry : fs::directory_iterator(this->args->input_file_path))
            {
                if (!fs::is_regular_file(entry))
                {
                    continue;
                }
                std::regex tenant_regex("^tenant_" + std::to_string(cur_tenant_id));
                std::string filename = entry.path().filename().string();
                std::string filepath = entry.path().string();
                if (std::regex_search(filename, inter_csv_regex) && std::regex_search(filename, tenant_regex))
                {
                    this->log_ofstream << "reading csv file: " << filename << std::endl;
                    INFO("reading csv file: %s\n", filename.c_str());
                    is_read = _read_res_csv_file(filepath, cur_tenant_id);
                }
            }
            if (!is_read)
            {
                ERROR("read_csv_files: Cannot find csv file for tenant %d\n", cur_tenant_id);
                abort();
            }
        }
    }

    bool DPSolver::_read_res_csv_file(std::string filepath, int tenant_id)
    {
        std::ifstream csv_file(filepath);
        if (!csv_file.is_open())
        {
            ERROR("Cannot open file: %s", filepath.c_str());
            return false;
        }
        std::string line;
        // ignore header
        std::getline(csv_file, line);
        while (std::getline(csv_file, line))
        {
            std::stringstream line_stream(line);
            std::string cell;
            std::vector<std::string> cells;
            while (std::getline(line_stream, cell, ','))
            {
                cells.push_back(cell);
            }
            if (cells.size() != 3) 
            {
                ERROR("read_csv_file: Invalid result csv file: %s", filepath.c_str());
                return false;
            }
            dp_entry_t entry;
            entry.cache_size = std::stol(cells[0]);
            entry.cache_size_in_gb = entry.cache_size / GIGABYTE_BASE;
            entry.bmrc_ratio = std::stod(cells[1]);
            entry.omrc_ratio = std::stod(cells[2]);
            this->dp_table[tenant_id-1].push_back(entry);
        }
        csv_file.close();
        return true;
    }

    void DPSolver::read_stat_csv_file()
    {
        std::ifstream csv_file(this->args->traffic_file_path);
        if (!csv_file.is_open())
        {
            ERROR("Cannot open file: %s", this->args->traffic_file_path.c_str());
            abort();
        }
        std::string line;
        // ignore header
        std::getline(csv_file, line);
        while (std::getline(csv_file, line))
        {
            std::stringstream line_stream(line);
            std::string cell;
            std::vector<std::string> cells;
            while (std::getline(line_stream, cell, ','))
            {
                cells.push_back(cell);
            }
            if (cells.size() != 5) 
            {
                ERROR("read_csv_file: Invalid stat csv file: %s", this->args->traffic_file_path.c_str());
                abort();
            }
            traffic_stat_t stat;
            int tenant_id = std::stoi(cells[0]);
            stat.total_request = std::stoull(cells[1]);
            stat.total_traffic = std::stoull(cells[2]);
            stat.unique_access = std::stoull(cells[3]);
            stat.unique_traffic = std::stoull(cells[4]);
            this->traffic_stats[tenant_id-1] = stat;
        }
        csv_file.close();
    }

    void DPSolver::solve()
    {
        // TODO: implement with dynamic programming
        // now using brute force
        std::vector<uint32_t> solved_cache_sizes;
        solved_cache_sizes.clear();
        uint64_t optimal_cost = UINT64_MAX;
        _greedy_search_solve(1, solved_cache_sizes, optimal_cost);
    }

    void DPSolver::_greedy_search_solve(int depth, std::vector<uint32_t> &solution, uint64_t &optimal_cost)
    {
        uint64_t no_cache_total_cost = 0;
        uint64_t allocated_total_cost = 0;
        std::vector<uint64_t> last_cost(this->n_server, 0);
        for(int i = 0; i < this->n_server; i++){
            last_cost[i] = this->traffic_stats[i].total_traffic;
            solution.push_back(0);
            no_cache_total_cost += this->traffic_stats[i].total_traffic;
        }

        for(int i = 0; i < this->allocation_space_in_gb; i++){
            uint64_t max_cost_decay = 0;
            int max_cost_decay_server = -1;
            //try allocate to one server
            for(int j = 0; j < this->n_server; j++){
                uint64_t cost_decay = last_cost[j] - this->dp_table[j][solution[j] + 1].bmrc_ratio * this->traffic_stats[j].total_traffic;
                // INFO("%d %d %ld\n", i, j, cost_decay);
                if(cost_decay > max_cost_decay){
                    // INFO("update\n");
                    max_cost_decay = cost_decay;
                    max_cost_decay_server = j;
                }
            }
            solution[max_cost_decay_server] += 1;
            // INFO("%d\n", max_cost_decay_server);
            // INFO("%d\n", solution[max_cost_decay_server]);
            // INFO("%f\n", this->dp_table[max_cost_decay_server][1].bmrc_ratio);
            last_cost[max_cost_decay_server] = this->dp_table[max_cost_decay_server][solution[max_cost_decay_server]].bmrc_ratio * this->traffic_stats[max_cost_decay_server].total_traffic;
        }
        for (int i=0; i<solution.size(); i++)
        {
            // printf("%d ", solution[i]);
            INFO("Solution: %d \n", solution[i]);
            allocated_total_cost += last_cost[i];
        }
        this->optimal_solution = solution;
        
        INFO("no cache total cost: %ld\n", no_cache_total_cost);
        INFO("allocated total cost: %ld\n", allocated_total_cost);
        INFO("miss ratio: %lf\n", 1.0*allocated_total_cost/no_cache_total_cost);
    }

    void DPSolver::_brute_force_solve(int depth, std::vector<uint32_t> &solution, uint64_t &optimal_cost)
    {
        if (depth > this->n_server)
        {
            uint64_t cost = _cost_cal(solution);
            if (cost < optimal_cost)
            {
                optimal_cost = cost;
                this->optimal_solution = solution;
            }
            return;
        }
        uint32_t allocated = 0;
        for (int i=0; i<solution.size(); i++)
        {
            INFO("%d ", solution[i]);
            allocated += solution[i];
        }
        INFO("allocated: %d\n", allocated);
        for (int i=0; i<this->allocation_space_in_gb; i++)
        {
            if (allocated + i > this->allocation_space_in_gb)
            {
                break;
            }
            solution.push_back(i);
            _brute_force_solve(depth+1, solution, optimal_cost);
            solution.pop_back();
        }
    }

    uint64_t DPSolver::_cost_cal(std::vector<uint32_t> solution)
    {
        uint64_t total_cost = 0;
        for (int i=0; i<this->n_server; i++)
        {
            total_cost += this->dp_table[i][solution[i]].bmrc_ratio * this->traffic_stats[i].total_traffic;
        }
        return total_cost;
    }
} // namespace CacheAllocation
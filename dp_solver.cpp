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

        read_csv_files();
    }

    DPSolver::~DPSolver()
    {
        this->log_ofstream.close();
    }

    void DPSolver::read_csv_files()
    {
        namespace fs  = std::experimental::filesystem;
        
        std::regex csv_regex("\\.csv$");
        for (int cur_tenant_id = 1; cur_tenant_id <= (int)this->n_server; cur_tenant_id++) 
        {
            bool is_read = false;
            for (const auto& entry : fs::directory_iterator(this->args->input_file_path))
            {
                if (!fs::is_regular_file(entry))
                {
                    continue;
                }
                std::regex tenant_regex("tenant_" + std::to_string(cur_tenant_id));
                std::string filename = entry.path().filename().string();
                if (std::regex_search(filename, csv_regex) && std::regex_search(filename, tenant_regex))
                {
                    is_read = _read_csv_file(filename, cur_tenant_id);
                }
            }
            if (!is_read)
            {
                ERROR("read_csv_files: Cannot find csv file for tenant %d", cur_tenant_id);
                abort();
            }
        }
    }
    bool DPSolver::_read_csv_file(std::string filepath, int tenant_id)
    {
        std::ifstream csv_file(filepath);
        if (!csv_file.is_open())
        {
            ERROR("Cannot open file: %s", filepath.c_str());
            return false;
        }
        std::string line;
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
                ERROR("read_csv_file: Invalid csv file: %s", filepath.c_str());
                return false;
            }
            dp_entry_t entry;
            entry.cache_size = std::stol(cells[0]);
            entry.cache_size_in_gb = entry.cache_size / GIGABYTE_BASE;
            entry.bmrc_ratio = std::stod(cells[1]);
            entry.omrc_ratio = std::stod(cells[2]);
            this->dp_table[tenant_id].push_back(entry);
        }
        csv_file.close();
        return true;
    }
} // namespace CacheAllocation
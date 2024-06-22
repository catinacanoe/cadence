#pragma once

#include "Block.h"

class Database {
private:
    std::vector<Block> block_list; // the list of blocks (sorted by start date)
    std::filesystem::path source_folder;
    std::string error_str;

    size_t index_at_time(time_t block_time);
    size_t rec_index_at_time(time_t block_time, size_t range_start, size_t range_end);
    void source_folder_integrity(std::filesystem::path val);
public:
    Database(Config* cfg_ptr);
    Database();

    void rename_block(time_t block_time, std::string new_title);
    void dump_info() const; // just for debug
    
    // sorted list of blocks with start dates within 24h of given date
    std::vector<Block> get_blocks_on_day(struct tm date) const;
};

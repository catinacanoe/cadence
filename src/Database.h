#pragma once

#include "Block.h"

class Database {
private:
    std::vector<Block> block_list; // the list of blocks (sorted by start date)
    std::filesystem::path source_folder;
    std::string error_str;

    void source_folder_integrity(std::filesystem::path val);
public:
    Database(std::filesystem::path save_folder);
    Database();

    void dump_info() const; // just for debug
    
    // sorted list of blocks with start dates within 24h of given date
    std::vector<Block> get_blocks_on_day(struct tm date) const;
};

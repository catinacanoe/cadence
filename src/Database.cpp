#include "Database.h"

Database::Database() {
    block_list = {};
    source_folder = "/";
    error_str = " ";
}

Database::Database(Config* cfg_ptr) {
    source_folder = cfg_ptr->str({"save_path"});
    source_folder_integrity(source_folder);

    error_str = "database in folder " + source_folder.string();

    for (const std::filesystem::path file
        : std::filesystem::directory_iterator(source_folder)) {
        if (!std::filesystem::is_regular_file(file)) continue;

        Block new_block = Block(file, cfg_ptr);

        bool inserted = false;
        for (size_t i = 0; i < block_list.size(); i++) {
            Block other_block = block_list[i];

            if (new_block.get_id() == other_block.get_id())
                throw std::runtime_error("two blocks have conflicting ids:\n"
                                         + new_block.get_source_file_str()+"\n"
                                         + other_block.get_source_file_str());

            if (new_block.get_time_t_start() == other_block.get_time_t_start())
                throw std::runtime_error("two blocks have conflicting start time:\n"
                                         + new_block.get_source_file_str()+"\n"
                                         + other_block.get_source_file_str());

            if (new_block.get_time_t_start() > other_block.get_time_t_start()) {
                continue; // then the new block should go after this one
            }
            
            block_list.insert(block_list.begin() + i, new_block);
            inserted = true;
            break;
        } if (!inserted) block_list.push_back(new_block);
    }
}

// public
std::vector<Block> Database::get_blocks_on_day(struct tm date) const {
    time_t start_time = std::mktime(&date);
    time_t end_time = start_time + 24*60*60;

    std::vector<Block> ret;

    bool hit_start = false;
    for (size_t i = 0; i < block_list.size(); i++) {
        Block block = block_list[i];

        if (!hit_start && block.get_time_t_start() >= start_time) hit_start = true;

        if (hit_start) {
            if (block.get_time_t_start() >= end_time) return ret;
            ret.push_back(block);
        }
    }

    return ret;
}

// private
size_t Database::index_at_time(time_t block_time) {
    return rec_index_at_time(block_time, 0, block_list.size()-1);
}

// private
size_t Database::rec_index_at_time(time_t block_time, size_t range_start, size_t range_end) {
    // recursive binary search
    if (range_start == range_end) throw std::runtime_error (error_str
         + ", error searching for block with start time: " + std::to_string(block_time)
         + ", range collapsed down to index: " + std::to_string(range_start));

    if (block_list[range_start].get_time_t_start() == block_time) return range_start;
    if (block_list[range_end].get_time_t_start() == block_time) return range_end;

    size_t pivot = (range_start + range_end) / 2;
    time_t pivot_time = block_list[pivot].get_time_t_start();

    if (block_time == pivot_time) return pivot;
    else if (block_time < pivot_time) return rec_index_at_time(block_time, range_start, pivot);
    else if (block_time > pivot_time) return rec_index_at_time(block_time, pivot, range_end);
    else return 0; // never hits
}

// public
void Database::rename_block(time_t block_time, std::string new_title) {
    size_t idx = index_at_time(block_time);

    block_list[idx].set_title(new_title);
    block_list[idx].save_to_file();
}

// private
void Database::source_folder_integrity(std::filesystem::path val) {
    if (!std::filesystem::is_directory(val)) throw std::runtime_error
        (error_str+", path is not valid");
}

// public
void Database::dump_info() const {
    for (const Block block : block_list) block.dump_info();
}

#include "Database.h"

Database::Database(std::filesystem::path save_folder) {
    source_folder_integrity(save_folder);
    source_folder = save_folder;

    error_str = "database in folder " + source_folder.string();

    for (const std::filesystem::path& file
        : std::filesystem::directory_iterator(source_folder)) {
        if (!std::filesystem::is_regular_file(file)) continue;

        Block new_block = Block(file);

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

void Database::source_folder_integrity(std::filesystem::path val) {
    if (!std::filesystem::is_directory(val)) throw std::runtime_error
        (error_str+", path is not valid");
}

void Database::dump_info() const {
    for (const Block& block : block_list) block.dump_info();
}
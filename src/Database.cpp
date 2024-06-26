#include "Database.h"

Database::Database(Config* cfg_ptr) {
    config_ptr = cfg_ptr;

    source_folder = config_ptr->str({"save_path"});
    error_str = "database in folder " + source_folder.string();
    source_folder_integrity(source_folder);

    for (const std::filesystem::path &file
        : std::filesystem::directory_iterator(source_folder)) {
        if (!std::filesystem::is_regular_file(file)) continue;

        insert_block(Block(file, config_ptr));
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
    return index_at_time_impl(block_time, 0, block_list.size()-1);
}

// private
size_t Database::index_at_time_impl(time_t block_time, size_t range_start, size_t range_end) {
    // recursive binary search
    if (range_start == range_end) throw std::runtime_error (error_str
         + ", error searching for block with start time: " + std::to_string(block_time)
         + ", range collapsed down to index: " + std::to_string(range_start));

    if (block_list[range_start].get_time_t_start() == block_time) return range_start;
    if (block_list[range_end].get_time_t_start() == block_time) return range_end;

    size_t pivot = (range_start + range_end) / 2;
    time_t pivot_time = block_list[pivot].get_time_t_start();

    if (block_time == pivot_time) return pivot;
    else if (block_time < pivot_time) return index_at_time_impl(block_time, range_start, pivot);
    else if (block_time > pivot_time) return index_at_time_impl(block_time, pivot, range_end);
    else return 0; // never hits
}

// public
void Database::rename_block(time_t block_time, std::string new_title) {
    int idx = index_at_time(block_time);

    Block old_block = block_list[idx];

    block_list[idx].set_title(new_title);
    block_list[idx].save_to_file();

    old_block.set_source_file(block_list[idx].get_source_file());

    undo_vec.push_back({ ACT_MODIFY, idx, old_block });
}

// private
int Database::fresh_id() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> id_range(1,99999);

    int ret = id_range(rng);

    for (Block block : block_list) {
        if (block.get_id() == ret)
            return fresh_id(); // try again
    }

    // if we got here, ret was never equal to a block's id
    // or else we would have hit the other return

    return ret;
}

// private
int Database::index_after_time(time_t block_time) {
    // TODO optimize into a recursive binary search
    for (size_t i = 0; i < block_list.size(); i++) {
        if (block_list[i].get_time_t_start() >= block_time) return i;
    }

    return -1;
}

// public
// precondition: time is the end time of a valid block, or day start if the day is empty
bool Database::new_block_below(time_t block_time) {
    // need to find the next block after this time
    Block block(config_ptr, fresh_id());
    block.set_time_t_start(block_time);
    block.set_duration(60*config_ptr->num({"time", "default_block_minutes"}));

    int next_idx = index_after_time(block_time);
    time_t this_day_end = block.get_date_time()
                        + 60*60*config_ptr->num({"time", "day_end_hour"})
                        + 60*config_ptr->num({"time", "day_end_minute"});

    if (next_idx == -1 || block_list[next_idx].get_time_t_start() >= block.get_time_t_end()) {
        // then the next block is not in the way
        // just check that we are not hitting day end
        if (block.get_time_t_start() >= this_day_end) {
            return false; // refuse to move previous task
        } else if (block.get_time_t_end() > this_day_end) {
            // just shorten block until it fits in the day
            block.set_duration(this_day_end - block.get_time_t_start());
        } else {
            // both times are before day_end
            // nothing can be before day_start bc this is ..._below()
            // all is good
        }
    } else if (block_list[next_idx].get_time_t_start() <= block.get_time_t_start()) {
        // this block is trying to start inside the next one, can't modify other blocks
        return false;
    } else if (block_list[next_idx].get_time_t_start() <= block.get_time_t_end()) {
        // this block ends inside the next block, shorten it
        block.set_duration(block_list[next_idx].get_time_t_start()
                           - block.get_time_t_start());
    } else {
        return false;
    }

    block.save_to_file();
    int idx = insert_block(block);

    undo_vec.push_back({ ACT_CREATE, idx, block });

    return true;
}

// public
std::tuple<time_t, int> Database::undo() {
    if (undo_vec.empty()) return {0, 0};

    // redo_vec.push_back(undo_action(undo_vec.back()));
    // undo_vec.pop_back();
    
    return undo_action(&undo_vec, &redo_vec);

    // return { redo_vec.back().block.get_time_t_start(), };
}

// public
std::tuple<time_t, int> Database::redo() {
    if (redo_vec.empty()) return {0, 0};

    // undo_vec.push_back(undo_action(redo_vec.back()));
    // redo_vec.pop_back();
    return undo_action(&redo_vec, &undo_vec);

    // return undo_vec.back().block.get_time_t_start();
}

// private
std::tuple<time_t, int> Database::undo_action(std::vector<struct action> *from,
                                        std::vector<struct action> *to) {
    struct action act = from->back();
    from->pop_back();

    Block block;

    switch (act.type) {
        case ACT_MODIFY:
            block = block_list[act.index];

            act.block.save_to_file();
            block_list[act.index] = act.block;

            // so that when this is written, it has the correct old source file
            block.set_source_file(act.block.get_source_file());
            act.block = block; // everything else remains the same
            to->push_back(act);
            break;
        case ACT_CREATE:
            // we need to delete the block
            // TODO figure out the fact that now the file is dead but block doesn't know
            block = block_list[act.index];
            block_list.erase(block_list.begin() + act.index);
            block.delete_file();

            to->push_back({ ACT_DELETE, 0, block });
            break;
        case ACT_DELETE:
            // we need to create the block
            // in theory if the undo was initialized correctly
            // the block's source file == "", so save_to_file should work

            block = act.block;

            block.save_to_file();
            int idx = insert_block(block);

            to->push_back({ ACT_CREATE, idx, block });
            break;
    }

    return { block.get_date_time(), block.get_id() };
}

// public
void Database::remove_block(time_t block_time) {
    int idx = index_at_time(block_time);

    Block old_block = block_list[idx];

    block_list.erase(block_list.begin() + idx);
    old_block.delete_file();

    undo_vec.push_back({ ACT_DELETE, 0, old_block });
}

size_t Database::insert_block(Block new_block) {
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
        return i;
    }
    
    block_list.push_back(new_block);
    return block_list.size() - 1;
}

// private
void Database::source_folder_integrity(std::filesystem::path val) {
    if (!std::filesystem::is_directory(val)) throw std::runtime_error
        (error_str+", path is not valid");
}

// public
void Database::dump_info() const {
    for (const Block &block : block_list) block.dump_info();
}

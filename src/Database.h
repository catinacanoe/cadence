#pragma once

#include "Block.h"

#include <random>
#include <tuple>

class Database {
public:
    Database(Config* cfg_ptr);

    void rename_block(time_t block_time, std::string new_title);
    bool new_block_below(time_t block_time); // returns whether successful or not
    bool new_block_above(time_t block_time); // returns whether successful or not
    bool move_block_up(time_t block_time); // return success
    bool move_block_down(time_t block_time); // return success
    bool move_block_lateral(time_t block_time, int amt); // return success
    void remove_block(time_t block_time);

    bool extend_top_up(time_t block_time);
    bool extend_top_down(time_t block_time);
    bool extend_bottom_up(time_t block_time);
    bool extend_bottom_down(time_t block_time);

    bool set_block_color(time_t block_time, std::string col);
    void block_toggle_important(time_t block_time);
    void block_toggle_collapsible(time_t block_time);

    std::tuple<time_t, int> undo(); // returns the time and id of the changing block
    std::tuple<time_t, int> redo();

    void dump_info() const; // just for debug
    
    // sorted list of blocks with start dates within 24h of given date
    std::vector<Block> get_blocks_on_day(struct tm date) const;

private:
    std::vector<Block> block_list; // the list of blocks (sorted by start date)
    std::filesystem::path source_folder;
    std::string error_str;
    Config* config_ptr;

    enum en_action_type { ACT_MODIFY, ACT_CREATE, ACT_DELETE };
    struct action {
        en_action_type type; // the type of action
        int index; // the index that was modified
        Block block; // the block as it was before the action
    };
    std::vector<struct action> undo_vec;
    std::vector<struct action> redo_vec;

    size_t index_at_time(time_t block_time);
    size_t index_at_time_impl(time_t block_time, size_t range_start, size_t range_end);
    int index_after_time(time_t block_time);
    int index_before_time(time_t block_time);
    void source_folder_integrity(std::filesystem::path val);
    int fresh_id();
    size_t insert_block(Block new_block);

    // undoes an action from the first vec (popping it)
    // then adds its opposite to the other vector
    // returns the date time and id of the block affected
    std::tuple<time_t, int> undo_action(std::vector<struct action> *from,
                                        std::vector<struct action> *to);

};

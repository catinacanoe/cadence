#pragma once

#include "Block.h"
#include "Database.h"

// represents one day, split up into time blocks (handles some ui)
class Day {
private:
    struct tm date; // the start of the day this object represents
    int line_count; // the number of lines this day is displayed on
    bool focused; // whether or not the day is focused
    bool is_today; // whether or not this block is today
    int focused_block; // which block is focused (index)
    time_t day_start, day_end; // the hours the day begins and ends (preconfigured)
    
    struct ui_block {
        Block block;
        int line_count; // the height of this block
    };
    std::vector<ui_block> ui_block_vec; // the list of blocks in this day
public:
    Day(Database *db_ptr, struct tm date_, time_t day_start_, time_t day_end_);

    void move_focus(int distance); // move the focus vertically (down positive)
    
    void draw(int height, int width, int top_y, int left_x); // draws the blocks within this x-coord bound
    
    void set_line_count(int new_line_count); // sets line count, recalculates block height
    void set_focused(bool new_focused); // set whether or not day is focused
};

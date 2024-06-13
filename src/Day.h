#pragma once

#include <ncurses.h>
#include "Block.h"
#include "Database.h"

// represents one day, split up into time blocks (handles some ui)
class Day {
private:
    struct tm date; // the start of the day this object represents
    Database *database_ptr; // pointer to the main task database
    int last_height; // the number of lines this day is displayed on
    bool focused; // whether or not the day is focused
    time_t day_start, day_end; // the hours the day begins and ends (preconfigured)
    std::string error_str;

    const char *date_format = "%d.%m";
    
    struct ui_block {
        Block block;
        bool focused; // whether or not this block is focused
        int top_y; // the y position of the top of this block (relative to this day's pos)
        int height; // the height of this block
    };
    std::vector<ui_block> ui_block_vec; // the list of blocks in this day
    
    bool is_today() const; // whether or not this day is today
    void resize_heights(int total_height); // sets line count, recalculates block height
    void populate_vector(); // using the db_ptr, load in today's tasks
public:
    Day(Database *db_ptr, struct tm date_, time_t day_start_, time_t day_end_);

    void move_focus(int distance); // move the focus vertically (down positive)
    void draw(int height, int width, int top_y, int left_x); // draws the blocks within this x-coord bound
    void set_focused(bool new_focused); // set whether or not day is focused
    void integrity_check() const;

    std::string get_date_str() const;
    void dump_info() const;
    
    struct tm get_date();
};

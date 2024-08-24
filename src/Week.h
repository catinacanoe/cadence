#pragma once

#include "Day.h"
#include "Database.h"

#include <limits>
#include <unordered_map>
#include <tuple>

// figuratively speaking. in reality it represents an arbitrary number of days
class Week {
private:
    Database *database_ptr; // pointer to the main task database
    Config *config_ptr; // pointer to the config table
    std::unordered_map<time_t, Day> day_map;
    Day* get_day(time_t date_time);
    void reload_day(time_t date_time);

    // struct tm start_date; // the day this 'week' start
    time_t start_date_time;
    time_t focused_date_time;
    int day_count;
    
    int last_total_width; // the last width that was given to resize
    int day_width, gap_width; // the width of the days and gaps between them (in columns)
    int target_day_width, target_gap_width; // the optimal day width we want to achieve (preconfigured)
    time_t day_start_t, day_end_t; // start and end times of the day (preconfigured)

    void resize_widths(int total_width); // resizes the sizing of colums to fit (returns number of days)
    void populate_vector(int day_count); // makes sure the vector has the correct size
    time_t get_end_date_time(); // get the datetime of the last day that is displayed
    void set_focus_inbounds(); // move the focus back into bounds if it wasn't
    void undo_redo_impl(std::tuple<time_t, int> tup); // move focus to this time
    bool move_block_lateral(int amt);
    Day* get_focused_day();
public:
    Week(Database *db_ptr, Config *cfg_ptr);
    Week();
    void move_focus(int distance); // focus the day this many away (right positive)
    void move_block_focus(int distance); // passed thru to the focused day

    // draws the week over the screen
    void draw(int height, int width, int y_corner, int x_corner);

    // getters
    Block get_focused_block();
    bool block_focused();

    // for modyfing blocks
    void rename_block(std::string new_title);
    void remove_block();
    void reload_all();
    bool new_block_below();
    bool new_block_above();

    bool move_block_down();
    bool move_block_up();
    bool move_block_right();
    bool move_block_left();

    bool extend_top_up();
    bool extend_top_down();
    bool extend_bottom_up();
    bool extend_bottom_down();

    bool set_block_color(std::string col);

    void block_toggle_important();
    void block_toggle_collapsible();

    void undo();
    void redo();

    // make sure the days are in order
    // run integrity check on each day
    void integrity_check() const;

    void dump_info() const; // debug
};

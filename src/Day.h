#pragma once

#include "Database.h"
#include "Config.h"

#include <ncursesw/ncurses.h>
#include <cmath>

// represents one day, split up into time blocks (handles some ui)
class Day {
private:
    struct tm date; // the start of the day this object represents
    
    Database *database_ptr;
    Config *config_ptr;

    bool highlighted; // whether or not the day is highlighted
    time_t day_start, day_end; // the hours the day begins and ends (preconfigured)
    std::string error_str;

    int last_height, last_width; // last height and width passed into resizing functions
    time_t last_time_per_line;
    
    std::string date_format;
    std::string day_format;
    
    int focused_block_idx; // the index of the focused uiblock

    struct ui_block {
        Block block;
        bool highlighted; // whether or not this block is highlighted
        bool bottom_adjacent; // whether or not there is a block adjacent to it below
        int top_y; // the y position of the top of this block (relative to this day's pos)
        int height; // the height of this block
        std::vector<std::string> title_vec; // the title split up into lines (line wrap)
    };
    std::vector<ui_block> ui_block_vec; // the list of blocks in this day

    bool is_today() const; // returns true if this date is today
    std::string get_relative_day() const; // returns "Yesterday" "Today" "Tomorrow"
                       // "Next/Last Week" "Next/Last Month" "Next/Last Year" or ""
    float get_line_at_time(time_t absolute_time); // returns the line number at unix tm
    void resize_heights(int total_height); // sets line count, recalculates block height
    void resize_width(int total_width); // rearranges the title line wrapping of blocks
    void draw_ui_block(struct ui_block uiblock, int height, // draw uiblock in given area
                       int width, int top_y, int left_x, bool focused);
    void draw_cursor(int top_y, int x_pos, bool focused); // draw marker at current time
    void draw_top_line(int width, int top_y, int left_x, bool focused); // date etc
    void draw_ui_block_title(struct ui_block uiblock, int height, int left_x, int top_y);
    void populate_vector(); // using the db_ptr, load in today's tasks

    void set_focus_inbounds(); // move the focus back into bounds if it wasn't
    
    enum en_box_type { BOX_NORMAL, BOX_IMPORTANT, BOX_BACKGROUND };
    void custom_box(int height, int width, int top_y, int left_x, en_box_type type, bool filled);
public:
    Day(Database *db_ptr, Config *cfg_ptr, time_t date_);

    void set_focus_line(int line); // focus the block closest to this line number
    int get_focus_line(); // get the currently focused line (approx)
    void move_focus(int distance);
    int get_focus();
    void set_focus(int new_focus);
    bool has_blocks();

    Block get_focused_block();
    int get_focus_time_start(); // return id of focused black

    void draw(int height, int width, int top_y, int left_x, bool focused); // draws the day in bounds
    void set_highlighted(bool new_highlighted); // set whether or not day is highlighted
    void integrity_check() const;
    
    bool is_date_equal(struct tm other) const;

    std::string get_date_str() const;
    std::string get_day_str() const;

    struct tm get_date() const;
    time_t get_date_time() const;
    bool get_highlighted() const;

    void dump_info() const;
    
};

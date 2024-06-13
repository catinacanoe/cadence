#pragma once

#include "Day.h"
#include "Database.h"
#include <limits>

// figuratively speaking. in reality it represents an arbitrary number of days
class Week {
private:
    Database *database_ptr; // pointer to the main task database
    std::vector<Day> day_vec; // the list of days
    
    struct tm start_date; // inclusive, the day this 'week' start
    
    int last_total_width; // the last width that was given to resize
    int day_width, gap_width; // the width of the days and gaps between them (in columns)
    int target_day_width, target_gap_width; // the optimal day width we want to achieve (preconfigured)
    time_t day_start_t, day_end_t; // start and end times of the day (preconfigured)

    int resize_widths(int total_width); // resizes the sizing of colums to fit (returns number of days)
    void populate_vector(int day_count); // makes sure the vector has the correct size
public:
    Week(Database *db_ptr, int target_day_width_, int target_gap_width_,
         time_t day_start_, time_t day_end_);
    void move_focus(int distance); // focus the day this many away (right positive)

    // draws the week over the screen
    void draw(int height, int width, int y_corner, int x_corner);

    // make sure the days are in order
    // run integrity check on each day
    void integrity_check() const;

    void dump_info() const; // debug
};

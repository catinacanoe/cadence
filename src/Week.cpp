#include "Week.h"

Week::Week() {
    database_ptr = nullptr;
    day_map = {};
    focused_date_time = start_date_time = 0;
    day_count = 0;
    last_total_width = day_width = gap_width = target_gap_width = target_gap_width
                     = day_start_t = day_end_t = 0;
}

Week::Week(Database *db_ptr, Config *cfg_ptr) {
    time_t now = time(0);
    struct tm tmp_tm = *std::localtime(&now);
    tmp_tm.tm_hour = tmp_tm.tm_min = tmp_tm.tm_sec = 0;
    focused_date_time = start_date_time = std::mktime(&tmp_tm);

    day_map = {};

    database_ptr = db_ptr;
    config_ptr = cfg_ptr;

    day_width = target_day_width = config_ptr->num({"ui", "target_day_width"});
    gap_width = target_gap_width = config_ptr->num({"ui", "target_gap_width"});

    day_start_t = 60*60*config_ptr->num({"time", "day_start_hour"})
                   + 60*config_ptr->num({"time", "day_start_minute"});

    day_end_t = 60*60*config_ptr->num({"time", "day_end_hour"})
                 + 60*config_ptr->num({"time", "day_end_minute"});
}

// public
void Week::draw(int height, int width, int top_y, int left_x) {
    // this will set the day and gap widths
    // and make sure the vector has all the days filled in
    resize_widths(width);

    // some day columns will be 'inflated' to eliminate empty columns on right of screen
    int empty_cols = width - (day_count * (day_width + gap_width) - gap_width);
    int small_inflation = empty_cols / day_count;
    int big_inflation = small_inflation + 1;
    int days_big_inflated = empty_cols - day_count * small_inflation;
    // ^^^ the amount of days inflated by big_inflation
    
    // go thru and draw all the days in the correct spot
    for (time_t i = start_date_time; i < start_date_time + day_count*24*60*60; i += 24*60*60) {
        int inflation = (i < days_big_inflated)? big_inflation : small_inflation;

        get_day(i)->draw(height, day_width + inflation, top_y, left_x, i == focused_date_time);
        left_x += day_width + inflation + gap_width;
    }
}

// public
bool Week::new_block_below() {
    time_t block_time;

    if (get_focused_day()->has_blocks())
        block_time = get_focused_day()->get_focused_block().get_time_t_end();
    else
        block_time = focused_date_time + day_start_t;

    if (database_ptr->new_block_below(block_time)) { // returns succesful bool
        reload_day(focused_date_time);
        get_focused_day()->move_focus(1);
        return true;
    } else return false;
}

// public
void Week::undo() {
    undo_redo_impl(database_ptr->undo());
    // const auto [datetime, id] = database_ptr->undo();
    // // time_t undotime = database_ptr->undo();
    // if (datetime == 0) return;

    // // set_focus_time(undotime + 60);

    // focused_date_time = datetime;
    // set_focus_inbounds();
    // reload_day(focused_date_time);
    // get_focused_day()->set_focus_id(id);
}

// public
void Week::redo() {
    undo_redo_impl(database_ptr->redo());
}

// private
void Week::undo_redo_impl(std::tuple<time_t, int> tup) {
    const auto [datetime, id] = tup;
    if (datetime == 0) return;

    focused_date_time = datetime;
    set_focus_inbounds();
    reload_day(focused_date_time);
    get_focused_day()->set_focus_id(id);
}

// public
bool Week::block_focused() { return get_focused_day()->has_blocks(); }

// public
Block Week::get_focused_block() { return get_focused_day()->get_focused_block(); }

// public
void Week::rename_block(std::string new_title) {
    database_ptr->rename_block(get_focused_day()->get_focus_time_start(), new_title);
    reload_day(focused_date_time);
}

// public
void Week::remove_block() {
    if (!block_focused()) return;

    database_ptr->remove_block(get_focused_day()->get_focus_time_start());
    reload_day(focused_date_time);
}

// private
void Week::reload_day(time_t date_time) {
    int focus = get_day(date_time)->get_focus();
    day_map.erase(day_map.find(focused_date_time));
    get_day(date_time)->set_focus(focus);
}

// private
Day* Week::get_day(time_t date_time) {
    if (day_map.find(date_time) == day_map.end()) {
        // the day at this time needs to be initilized
        day_map.insert(std::make_pair(
            date_time, Day(database_ptr, config_ptr, date_time)));
    }

    return &day_map.at(date_time); // segfault
}

// private
time_t Week::get_end_date_time() { return start_date_time + (day_count-1)*24*60*60; }

// public
void Week::integrity_check() const {
    for (const auto & [ date, day ] : day_map) {
        if (date != day.get_date_time()) throw std::runtime_error
            ("Week integrity_check: Day at time " + std::to_string(date)
             + " has incorrect yday: " + std::to_string(day.get_date_time()));

        day.integrity_check();
    }
}

// public
void Week::move_focus(int distance) {
    int line = get_focused_day()->get_focus_line();
    focused_date_time += distance*24*60*60;
    get_focused_day()->set_focus_line(line);

    set_focus_inbounds();
}

// private
void Week::set_focus_inbounds() {
    int start_diff = focused_date_time - start_date_time;
    int end_diff = get_end_date_time() - focused_date_time;

    if (start_diff < 0)
        start_date_time += start_diff;
    else if (end_diff < 0)
        start_date_time -= end_diff;
}

// private
Day* Week::get_focused_day() { return get_day(focused_date_time); }

// public
void Week::move_block_focus(int distance) { 
    get_day(focused_date_time)->move_focus(distance);
}

// private
void Week::resize_widths(int total_width) {
    if (last_total_width == total_width) return;

    last_total_width = total_width;

    day_width = target_day_width;
    gap_width = target_gap_width;
    float best_rating = std::numeric_limits<float>::max();

    float numerator = total_width + target_day_width;
    float denominator = target_gap_width + target_day_width;
    day_count = (int) (numerator / denominator + 0.5);

    for (int loop_day_width = target_day_width / 2; loop_day_width < 2*target_day_width; loop_day_width++) {
        for (int loop_gap_width = target_gap_width / 2; loop_gap_width <
        std::max(day_width / 4, target_gap_width / 2 + 1); loop_gap_width++) {

            int empty_cols = total_width - (day_count * (loop_day_width + loop_gap_width) - loop_gap_width);
            if (empty_cols < 0) continue; // the configuration overflows screen
            
            int target_dev = (std::abs(target_day_width - loop_day_width)
                           + std::abs(target_gap_width - loop_gap_width)) / 2;

            // lower is better
            float closeness_rating = 1.0 * (float) target_dev
                                   + 3.0 * (float) empty_cols;

            if (closeness_rating < best_rating) {
                day_width = loop_day_width;
                gap_width = loop_gap_width;
                best_rating = closeness_rating;
            }
        }
    }

    // if no configuration fit in the screen
    if (best_rating == std::numeric_limits<float>::max()) {
        day_count = 1;
        day_width = total_width;
        gap_width = 0;
    }

    set_focus_inbounds();
}

//public
void Week::dump_info() const {
    std::cout << " - Week::dump_info()" << std::endl;
    std::cout << "start date time: " << start_date_time << std::endl;
    std::cout << "focused date time: " << focused_date_time << std::endl;
    std::cout << "day count: " << day_count << std::endl;
    std::cout << "last tot width: " << last_total_width << std::endl;
    std::cout << "day width: " << day_width << std::endl;
    std::cout << "gap width: " << gap_width << std::endl;
    std::cout << "target day width: " << target_day_width << std::endl;
    std::cout << "target gap width: " << target_gap_width << std::endl;
    std::cout << "day start: " << day_start_t << std::endl;
    std::cout << "day end: " << day_end_t << std::endl;

    if (day_map.empty()) {
        std::cout << "no days in vector" << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << "LIST OF DAYS:" << std::endl;
    for (const auto & [ date, day ] : day_map) {
        std::cout << std::endl;
        day.dump_info();
    }
    std::cout << std::endl << "END OF DAYS" << std::endl;
}

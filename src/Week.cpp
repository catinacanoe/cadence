#include "Week.h"

Week::Week() {
    database_ptr = nullptr;
    day_vec = {};
    start_date = {0};
    last_total_width = day_width = gap_width = target_gap_width = target_gap_width
                     = day_start_t = day_end_t = 0;
}

Week::Week(Database *db_ptr, int target_day_width_, int target_gap_width_,
           time_t day_start_t_, time_t day_end_t_) {
    time_t now = time(0);
    start_date = *localtime(&now);
    start_date.tm_hour = start_date.tm_min = start_date.tm_sec = 0;

    day_vec = {};

    database_ptr = db_ptr;
    day_width = target_day_width = target_day_width_;
    gap_width = target_gap_width = target_gap_width_;
    day_start_t = day_start_t_;
    day_end_t = day_end_t_;
}

// public
void Week::draw(int height, int width, int y_corner, int x_corner) {
    // this will set the day and gap widths
    // and make sure the vector has all the days filled in
    populate_vector(resize_widths(width));

    int day_count = day_vec.size();
    int empty_cols = width - (day_count * (day_width + gap_width) - gap_width);

    int small_inflation = empty_cols / day_count;
    int big_inflation = small_inflation + 1;
    int days_big_inflated = empty_cols - day_count * small_inflation;
    // ^^^ the amount of days inflated by big_inflation
    
    int x = x_corner;
    
    // go thru and draw all the days in the correct spot
    for (size_t i = 0; i < day_count; i++) {
        // move the empty columns into the day_width (to not waste space)
        int inflation = (i < days_big_inflated)? big_inflation : small_inflation;

        day_vec[i].draw(height, day_width + inflation, y_corner, x);
        x += day_width + inflation + gap_width;
    }
}

// public
void Week::integrity_check() const {
    for (size_t i = 0; i < day_vec.size(); i++) {
        Day day = day_vec[i];

        int expected = start_date.tm_yday + i;
        int real = day.get_date().tm_yday; 
        if (real != expected) throw std::runtime_error
            ("Day at index " + std::to_string(i) + " has incorrect yday: "
             + std::to_string(real) + " instead of " + std::to_string(expected));

        day.integrity_check();
    }
}

// private
void Week::populate_vector(int day_count) {
    if (day_count < day_vec.size()) { // remove the extra days
        day_vec.erase(day_vec.begin() + day_count, day_vec.end());
    } else if (day_count > day_vec.size()) { // add the missing days
        for (size_t i = day_vec.size(); i < day_count; i++) {
            // struct tm new_date = start_date; new_date.tm_day += i;
            time_t new_time = i*24*60*60 + std::mktime(&start_date);
            struct tm new_date = *std::localtime(&new_time);
            day_vec.push_back(Day(database_ptr, new_date, day_start_t, day_end_t));
        }
    }

    integrity_check();
}

//public
void Week::dump_info() const {
    std::cout << " - Week::dump_info()" << std::endl;
    std::cout << "last tot width: " << last_total_width << std::endl;
    std::cout << "day width: " << day_width << std::endl;
    std::cout << "gap width: " << gap_width << std::endl;
    std::cout << "target day width: " << target_day_width << std::endl;
    std::cout << "target gap width: " << target_gap_width << std::endl;
    std::cout << "day start: " << day_start_t << std::endl;
    std::cout << "day end: " << day_end_t << std::endl;

    if (day_vec.size() == 0) {
        std::cout << "no days in vector" << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << "LIST OF DAYS:" << std::endl;
    for (Day day : day_vec) {
        std::cout << std::endl;
        day.dump_info();
    }
    std::cout << std::endl << "END OF DAYS" << std::endl;
}

// private
int Week::resize_widths(int total_width) {
    if (last_total_width == total_width) return day_vec.size();

    last_total_width = total_width;

    int best_day_width = target_day_width;
    int best_gap_width = target_gap_width;
    float best_rating = std::numeric_limits<float>::max();

    float numerator = total_width + target_day_width;
    float denominator = target_gap_width + target_day_width;
    int day_count = (int) (numerator / denominator + 0.5);

    for (day_width = target_day_width / 2; day_width < 2*target_day_width; day_width++) {
        for (gap_width = target_gap_width / 2; gap_width < day_width / 4; gap_width++) {

            int empty_cols = total_width - (day_count * (day_width + gap_width) - gap_width);
            if (empty_cols < 0) continue; // the configuration overflows screen
            
            int target_dev = (std::abs(target_day_width - day_width)
                           + std::abs(target_gap_width - gap_width)) / 2;

            // lower is better
            float closeness_rating = 1.0 * (float) target_dev
                                   + 3.0 * (float) empty_cols;

            if (closeness_rating < best_rating) {
                best_day_width = day_width;
                best_gap_width = gap_width;
                best_rating = closeness_rating;
            }
        }
    }

    if (best_rating == std::numeric_limits<float>::max()) {
        // no configuration fit in the screen
        day_width = total_width;
        gap_width = 0;
        return 1;
    }

    day_width = best_day_width;
    gap_width = best_gap_width;

    return day_count;
}

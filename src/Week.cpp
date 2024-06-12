#include "Week.h"

Week::Week(Database *db_ptr, int target_day_width_, int target_gap_width_,
           time_t day_start_t_, time_t day_end_t_) {
    time_t now = time(0);
    start_date = *localtime(&now);
    start_date.tm_hour = start_date.tm_min = start_date.tm_sec = 0;

    database_ptr = db_ptr;
    day_width = target_day_width = target_day_width_;
    gap_width = target_gap_width = target_gap_width_;
    day_start_t = day_start_t_;
    day_end_t = day_end_t_;
}

void Week::draw(int height, int width, int y_corner, int x_corner) {
    // this will set the day and gap widths
    // and make sure the vector has all the days filled in
    populate_vector(resize_widths(width));

    // go thru and draw all the days in the correct spot
    for (size_t i = 0; i < day_vec.size(); i++) {
        Day day = day_vec[i];

        day.draw(height, day_width, y_corner, x_corner + i*(day_width+gap_width));
    }
}

void integrity_check() {
    // TODO
}

void populate_vector(int day_count) {
    if (day_count < day_vec.size()) { // remove the extra days
        day_vec.erase(day_vec.begin() + day_count, day_vec.end());

    } else if (day_count > day_vec.size()) { // add the missing days
        for (size_t i = day_vec.size(); i < day_count; i++) {
            struct tm new_date = start_date; new_date.tm_day += i;
            day_vec.push_back(Day(database_ptr, new_date, day_start_t, day_end_t));
        }
    }
}

int Week::resize_widths(int total_width) {
    // if the current setup works (width hasn't changed, return current value)
    if (day_vec.size() * (day_width + gap_width) - gap_width == total_width)
        return day_vec.size();

    int best_day_width = day_width = target_day_width;
    int best_gap_width = gap_width = target_gap_width;
    float best_rating = 100.0;

    float numerator = total_width + target_day_width;
    float denominator = target_gap_width + target_day_width;
    int day_count = (int) (numerator / denominator + 0.5);

    // loop thru day widths, deviating from target value by a maximum of 50%
    for (int day_width_dev = 0; day_width_dev < target_day_width; day_width_dev++) {
        if (day_width_dev%2) day_width -= day_width_dev;
        else day_width += day_width_dev;

        // loop thru gap widths, deviating from target value by a maximum of 50%
        for (int gap_width_dev = 0; gap_width_dev < target_gap_width; gap_width_dev++) {
            if (gap_width_dev%2) gap_width -= gap_width_dev;
            else gap_width += gap_width_dev;

            int empty_cols = total_width - (day_count * (day_width + gap_width) - gap_width);
            int target_dev = (day_width_dev + gap_width_dev) / 2;

            // lower is better
            float closeness_rating = 1.0*(float)target_dev + 1.0*(float)empty_cols;

            if (closeness_rating < best_rating) {
                best_day_width = day_width;
                best_gap_width = gap_width;
                best_rating = closeness_rating;
            }
        }
    }

    day_width = best_day_width;
    gap_width = best_gap_width;

    return day_count;
}

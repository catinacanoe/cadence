#include "Day.h"

Day::Day(Database *db_ptr, struct tm date_, time_t day_start_, time_t day_end_) {
    date = date_;
    database_ptr = db_ptr;
    last_height = 0;
    focused = false;
    day_start = day_start_;
    day_end = day_end_;
    error_str = "Day on " + std::to_string(date.tm_mday) + "."
                          + std::to_string(date.tm_mon) + "."
                          + std::to_string(date.tm_year);

    populate_vector();
}

// public
void Day::draw(int height, int width, int top_y, int left_x) {
    resize_heights(height);

    for (struct ui_block uiblock : ui_block_vec) {
        std::string str = uiblock.block.get_title() + " " + uiblock.block.get_t_start_hour_str();
        mvprintw(top_y + uiblock.top_y, left_x, "%s", str.c_str());
        mvprintw(top_y + uiblock.top_y + uiblock.height - 1, left_x, "%s", "----");
    }
}

// private
void Day::populate_vector() {
    std::vector<int> focused_ids;
    // find the id of the focused block(s)
    for (struct ui_block uiblock : ui_block_vec)
        if (uiblock.focused) focused_ids.push_back(uiblock.block.get_id());

    ui_block_vec.clear();

    bool is_first = true;
    for (Block block : database_ptr->get_blocks_on_day(date)) {
        std::cout << block.get_title() << std::endl;
        struct ui_block new_ui_block = { block, false, 0, 0 };

        if (focused_ids.size() == 0 && is_first) new_ui_block.focused = true;

        // if this block's id is in the focused list
        if (std::find(focused_ids.begin(), focused_ids.end(), block.get_id())
            != focused_ids.end()) new_ui_block.focused = true;

        ui_block_vec.push_back(new_ui_block);

        is_first = false;
    }
}

// private
void Day::resize_heights(int total_height) {
    if (total_height == last_height) return;
    last_height = total_height;
    time_t total_time = day_end - day_start;

    // account for collapsed tasks not requiring space for their time
    for (struct ui_block uiblock : ui_block_vec)
        if (uiblock.block.get_collapsible()) total_time -= uiblock.block.get_duration();

    // each block has an upper and lower border
    // so only the remaining space can be used to express time length
    int total_lines = total_height - 2 * ui_block_vec.size();

    // the amount of time each line represents
    time_t time_per_line = total_time / total_lines;

    // calculate and assign height and position to blocks
    time_t last_end_time = std::mktime(&date) + day_start;
    int last_end_line = 0;
    for (struct ui_block& uiblock : ui_block_vec) {
        if (uiblock.block.get_collapsible()) {
            uiblock.top_y = last_end_line;
            uiblock.height = 2;
        } else {
            float f_top_y = uiblock.block.get_time_t_start() - last_end_time;
            f_top_y /= time_per_line;

            uiblock.top_y = (int) (last_end_line + f_top_y + 0.5);

            float f_height = uiblock.block.get_duration();
            f_height /= time_per_line;
            uiblock.height = (int) (2 + f_height + 0.5);
        }

        last_end_line = uiblock.top_y + uiblock.height;
        last_end_time = uiblock.block.get_time_t_start() + uiblock.block.get_duration();
    }

    time_t extra_time = mktime(&date) + day_end - last_end_time; // from last task to eod
    bool is_extra_time = extra_time > 0;
    if (extra_time > 0) {
        float extra_lines = extra_time;
        extra_lines /= time_per_line;
        last_end_line += (int) (extra_lines + 0.5);
    }

    // check that the heights add up
    int empty_rows = total_height - last_end_line; // rows left empty
    int bottom_y = 0;

    // if there are empty rows or overflow, account for it
    // in theory this should never happen but just in case
    // multiple passes, so that extreme measures only take if necessary
    for (int pass = 0; pass <= 4; pass++) {
        for (size_t i = 0; i < ui_block_vec.size(); i++) {
            struct ui_block& uiblock = ui_block_vec[i];
            int prev_bottom_y = bottom_y;
            bottom_y = uiblock.top_y + uiblock.height;

            if (empty_rows == 0) break; // job done - this will usually be hit first iter
            
            int adjustment;
            
            if (pass == 4) {
                adjustment = empty_rows;
                int adj_bound = 2 - uiblock.height;
                if (adjustment < adj_bound) adjustment = adj_bound;
                empty_rows -= adjustment;

                uiblock.height += adjustment;
                for (size_t j = i+1; j < ui_block_vec.size(); j++)
                    ui_block_vec[j].top_y += adjustment;

                continue;
            }

            // find blocks with gap before them
            if (prev_bottom_y == uiblock.top_y) continue;

            int gap_size = uiblock.top_y - prev_bottom_y;
            // if on the first pass, only use gaps bigger than s
            if (gap_size <= 2 - pass) continue;

            // move things up or down
            if (empty_rows > 0) adjustment = (pass < empty_rows)? pass : empty_rows;
            else adjustment = (pass < -empty_rows)? -pass : empty_rows;

            if (pass == 3) adjustment = empty_rows;
            if (-adjustment > gap_size) adjustment = -gap_size;

            empty_rows -= adjustment;
            for (size_t j = i; j < ui_block_vec.size(); j++)
                ui_block_vec[j].top_y += adjustment;
        }
    }
}

// public
void Day::dump_info() const {
    std::cout << " - Day::dump_info()" << std::endl;
    std::cout << "date: " << get_date_str() << std::endl;
    std::cout << "last height: " << last_height << std::endl;
    std::cout << "focused: " << focused << std::endl;
    std::cout << "day start: " << day_start << std::endl;
    std::cout << "day end: " << day_end << std::endl;

    if (ui_block_vec.size() == 0) {
        std::cout << "no blocks in vector" << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << "LIST OF BLOCKS:" << std::endl;
    for (struct ui_block uiblock : ui_block_vec) {
        std::cout << std::endl;
        std::cout << "top_y: " << uiblock.top_y << ", height: " << uiblock.height << ", focused: " << uiblock.focused << std::endl;
        uiblock.block.dump_info();
    }
    std::cout << std::endl << "END OF BLOCKS" << std::endl;
}

// public
std::string Day::get_date_str() const {
    char buffer[6];
    struct tm copy = date;
    std::strftime(buffer, sizeof(buffer), date_format, &copy);

    return std::string(buffer);
}

// public
void Day::move_focus(int distance) {

}

// public
void Day::set_focused(bool new_focused) {

}

// public
void Day::integrity_check() const {
    if (date.tm_hour != 0 || date.tm_min != 0 || date.tm_sec != 0)
        throw std::runtime_error(error_str + ", date is not zeroed to midnight");
}

// private
bool Day::is_today() const {
    time_t now = time(0);
    struct tm today = *localtime(&now);

    return date.tm_yday == today.tm_yday && date.tm_year == today.tm_year;
}

struct tm Day::get_date() { return date; }

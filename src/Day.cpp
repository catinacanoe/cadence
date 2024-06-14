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
    resize_heights(height - 1); // one line used for date
    
    std::string date_str = get_date_str();
    mvprintw(top_y, left_x + width - date_str.size(), "%s", date_str.c_str());

    std::string rel_day = get_relative_day();
    if (rel_day.size() != 0)
        mvprintw(top_y, left_x, "%s", rel_day.c_str());

    top_y++;
    
    for (struct ui_block uiblock : ui_block_vec) {
        int block_top_y = top_y + uiblock.top_y;

        WINDOW *block_border = newwin(uiblock.height, width, block_top_y, left_x);

        if (uiblock.block.get_important()) {
            double_box(block_border);
        } else {
            box(block_border, 0, 0);
        }

        wrefresh(block_border);
        delwin(block_border);

        std::string hour = uiblock.block.get_t_start_hour_str();
        mvprintw(block_top_y, left_x + 1, "%s", hour.c_str());
        refresh();

        if (uiblock.block.get_collapsible()) {
            mvprintw(block_top_y, left_x + width - 1, "%s", ".");
            refresh();
        }

        std::string title = uiblock.block.get_title();
        int title_y = block_top_y;
        if (uiblock.height == 2) title_y += uiblock.height - 1;
        else title_y += (uiblock.height - 1) / 2;

        mvprintw(title_y, left_x + width - title.size() - 2, "%s", title.c_str());
    }

    if (ui_block_vec.size() == 0) {
        WINDOW *block_border = newwin(height - 1, width, top_y, left_x);
        box(block_border, 0, 0);
        wrefresh(block_border);
    }

    refresh();
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
    float f_tpl = total_time;
    f_tpl /= total_lines;
    time_t time_per_line = (time_t) (f_tpl + 0.5);
    last_time_per_line = time_per_line;

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
    std::cout << "date format: " << date_format << std::endl;
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
    char buffer[20];
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
std::string Day::get_relative_day() const {
    time_t now = time(0);
    struct tm today = *localtime(&now);
    today.tm_hour = 0; today.tm_min = 0; today.tm_sec = 0;
    struct tm date_cp = date;

    time_t today_t = std::mktime(&today);
    time_t date_t = std::mktime(&date_cp);
    time_t day = 24*60*60;

    if (date_t == today_t - day) return "Yesterday";
    else if (date_t == today_t) return "Today";
    else if (date_t == today_t + day) return "Tomorrow";
    else if (date_t == today_t + 7*day) return "Next Week";
    else if (date_t == today_t - 7*day) return "Last Week";

    else if (date.tm_mday == today.tm_mday && date.tm_year == today.tm_year) {
        if (date.tm_mon == today.tm_mon + 1) return "Next Month";
        else if (date.tm_mon == today.tm_mon - 1) return "Last Month";
        else return "";
    } else if (date.tm_mday == today.tm_mday && date.tm_mon == today.tm_mon) {
        if (date.tm_year == today.tm_year + 1) return "Next Year";
        else if (date.tm_year == today.tm_year - 1) return "Last Year";
        else return "";
    } else return "";
}

// private
void Day::double_box(WINDOW *win) {
    int height, width, top_y, left_x;
    getmaxyx(win, height, width);
    getbegyx(win, top_y, left_x);

    std::string horizontal = "";
    for (int i = 0; i < width - 2; i++) horizontal += "═";
    std::string top = "╔" + horizontal + "╗";
    std::string bottom = "╚" + horizontal + "╝";
    std::string vertical = "║";

    mvprintw(top_y, left_x, "%s", top.c_str());
    mvprintw(top_y + height - 1, left_x, "%s", bottom.c_str());

    for (int i = 1; i < height - 1; i++) {
        mvprintw(top_y + i, left_x, "%s", vertical.c_str());
        mvprintw(top_y + i, left_x + width - 1, "%s", vertical.c_str());
    }
}

// public
struct tm Day::get_date() { return date; }

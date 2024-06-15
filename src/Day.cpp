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
    if (ui_block_vec.size() == 0) return;

    resize_heights(height - 1); // one line used for date
    
    std::string date_str = get_date_str();
    mvprintw(top_y, left_x + width - date_str.size(), "%s", date_str.c_str());

    std::string rel_day = get_relative_day();
    if (rel_day.size() != 0)
        mvprintw(top_y, left_x, "%s", rel_day.c_str());

    top_y++;

    attron(COLOR_PAIR(7));
    custom_box(height - 1, width, top_y, left_x, 2, false);
    attroff(COLOR_PAIR(7));

    for (struct ui_block uiblock : ui_block_vec) {
        int block_top_y = top_y + uiblock.top_y;


        attron(COLOR_PAIR(uiblock.block.get_color()));
        custom_box(uiblock.height, width, block_top_y, left_x,
                   uiblock.block.get_important()? 1 : 0, uiblock.block.get_collapsible());
        attroff(COLOR_PAIR(uiblock.block.get_color()));
        refresh();

        std::string hour = uiblock.block.get_t_start_hour_str();
        mvprintw(block_top_y, left_x + 1, "%s", hour.c_str());
        refresh();

        if (!uiblock.bottom_adjacent) {
            hour = uiblock.block.get_t_end_hour_str();
            int draw_height = (uiblock.height == 2)? 0 : uiblock.height - 1;
            draw_height += block_top_y;

            mvprintw(draw_height, left_x + width - hour.size() - 1, "%s", hour.c_str());
            refresh();
        }

        std::string title = uiblock.block.get_title();
        int title_y = block_top_y;
        int title_x = left_x + 1;
        if (uiblock.height == 2) { title_y += uiblock.height - 1; }
        else { title_y += (uiblock.height - 1) / 2; title_x++; }

        mvprintw(title_y, title_x, "%s", title.c_str());
    }

    refresh();

    if (is_today()) {
        float f_line = get_line_at_time(time(0));
        int i_line = (int) f_line;
        int dec_3 = (int) (3 * (f_line - i_line));

        std::string character = "X";
        if      (dec_3 == 0) character = "🬂";
        else if (dec_3 == 1) character = "🬋";
        else if (dec_3 == 2) character = "🬭";

        attron(COLOR_PAIR(3));
        mvprintw(top_y + i_line, left_x + width, "%s", character.c_str());
        attroff(COLOR_PAIR(3));
    }
}

// private
void Day::populate_vector() {
    std::vector<int> focused_ids;
    // find the id of the focused block(s)
    for (struct ui_block uiblock : ui_block_vec)
        if (uiblock.focused) focused_ids.push_back(uiblock.block.get_id());

    ui_block_vec.clear();

    for (Block block : database_ptr->get_blocks_on_day(date)) {
        struct ui_block new_ui_block = { block, false, false, 0, 0 };

        // if this block's id is in the focused list
        if (std::find(focused_ids.begin(), focused_ids.end(), block.get_id())
            != focused_ids.end()) new_ui_block.focused = true;

        if (ui_block_vec.size() != 0) {
            struct ui_block &last = ui_block_vec.back();

            last.bottom_adjacent =
                last.block.get_time_t_start() + last.block.get_duration()
                == block.get_time_t_start();
        }

        ui_block_vec.push_back(new_ui_block);
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
        float f_top_y = uiblock.block.get_time_t_start() - last_end_time;
        f_top_y /= time_per_line;

        uiblock.top_y = (int) (last_end_line + f_top_y + 0.5);

        if (uiblock.block.get_collapsible()) {
            uiblock.height = 2;
        } else {
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

// private
float Day::get_line_at_time(time_t absolute_time) { // assume the time is today
    absolute_time -= (absolute_time % 60); // remove seconds
    // first find what task it falls into
    float start_line = 0, height = 0;
    time_t start_time = 0, duration = 0;

    int previous_end_line;
    time_t previous_end_time;
    for (struct ui_block uiblock: ui_block_vec) { Block block = uiblock.block;

        // first check range between last block and start of this one
        if (absolute_time > previous_end_time &&
            absolute_time < block.get_time_t_start()) {

            start_line = previous_end_line - 0.33333;
            height = uiblock.top_y - previous_end_line + 0.66666;
            start_time = previous_end_time;
            duration = block.get_time_t_start() - previous_end_time;

            break;
        // now check the range of this block
        } else if (absolute_time >= block.get_time_t_start() &&
                   absolute_time <= block.get_time_t_start() + block.get_duration()) {

            start_line = uiblock.top_y + 0.5;
            height = uiblock.height - 1;
            start_time = block.get_time_t_start();
            duration = block.get_duration();

            break;
        }

        previous_end_line = uiblock.top_y + uiblock.height;
        previous_end_time = block.get_time_t_start() + block.get_duration();
    }

    if (height == 0) { // time is below the last block
        struct ui_block uiblock = ui_block_vec.back();

        start_line = uiblock.top_y + uiblock.height - 0.33333;
        height = last_height - previous_end_line + 0.33333;
        start_time = previous_end_time;
        duration = std::mktime(&date) + day_end - previous_end_time;
    }

    // ret = start_line + height * (absolute_time - start_time) / duration
    float ret = height;
    ret /= duration;
    ret *= absolute_time - start_time;
    ret += start_line;

    return ret;
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
void Day::set_focused(bool new_focused) { focused = new_focused; }

// public
void Day::integrity_check() const {
    if (date.tm_hour != 0 || date.tm_min != 0 || date.tm_sec != 0)
        throw std::runtime_error(error_str + ", date is not zeroed to midnight");
}

// private
bool Day::is_today() const {
    // struct tm date_cp = date;
    time_t now = time(0);
    struct tm today = *localtime(&now);
    // today.tm_hour = 0; today.tm_min = 0; today.tm_sec = 0;

    return date.tm_year == today.tm_year
        && date.tm_mon == today.tm_mon
        && date.tm_mday == today.tm_mday;
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
void Day::custom_box(int height, int width, int top_y, int left_x, int type, bool collapsed) {
    if (type < 0 || type > 2)
        throw std::runtime_error("Day::custom_box type entered out of range");

    std::string tl, tr, bl, br, hz, vr;
    if (type == 0) { // normal box
        tl = "┌";
        // tr = collapsed ? "⋅" : "┐";
        // tr = collapsed ? "─" : "┐";
        tr = collapsed ? "╴" : "┐";
        bl = "└";
        br = "┘";

        hz = "─";
        vr = "│";
    } else if (type == 1) { // double border
        tl = "╔";
        // tr = collapsed ? "⋅" : "╗";
        tr = collapsed ? "═" : "╗";
        bl = "╚";
        br = "╝";

        hz = "═";
        vr = "║";
    } else if (type == 2) { // vertical dashed column
        hz = " ";

        tl = tr = bl = br = vr = "┊";
        // tl = tr = bl = br = vr = "┆";
        // tl = tr = bl = br = vr = "╎";
    }

    std::string horizontal = "";
    for (int i = 0; i < width - 2; i++) horizontal += hz;
    std::string top = tl + horizontal + tr;
    std::string bottom = bl + horizontal + br;

    mvprintw(top_y, left_x, "%s", top.c_str());
    mvprintw(top_y + height - 1, left_x, "%s", bottom.c_str());

    for (int i = 1; i < height - 1; i++) {
        mvprintw(top_y + i, left_x, "%s", vr.c_str());
        mvprintw(top_y + i, left_x + width - 1, "%s", vr.c_str());
    }
}

// public
struct tm Day::get_date() { return date; }

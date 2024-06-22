#include "Day.h"

Day::Day(Database *db_ptr, Config *cfg_ptr, time_t date_) {
    date = *std::localtime(&date_);

    database_ptr = db_ptr;
    config_ptr = cfg_ptr;

    last_height = 0;
    highlighted = false;
    focused_block_idx = 0;

    day_start = 60*60*config_ptr->num({"time", "day_start_hour"})
                 + 60*config_ptr->num({"time", "day_start_minute"});

    day_end = 60*60*config_ptr->num({"time", "day_end_hour"})
               + 60*config_ptr->num({"time", "day_end_minute"});

    date_format = config_ptr->str({"ui", "date_formats", "date_format"});
    day_format = config_ptr->str({"ui", "date_formats", "day_format"});

    error_str = "Day on " + std::to_string(date.tm_mday) + "."
                          + std::to_string(date.tm_mon) + "."
                          + std::to_string(date.tm_year);

    populate_vector();
}

// public
void Day::draw(int height, int width, int top_y, int left_x, bool focused) {
    resize_heights(height - 1); // one line used for top_line (date str)
    resize_width(width);

    // draw the vertical rails bounding the day
    attron(COLOR_PAIR(config_ptr->num({"ui", "colors", "background"})));
    custom_box(height, width, top_y, left_x, BOX_BACKGROUND, false);
    attroff(COLOR_PAIR(config_ptr->num({"ui", "colors", "background"})));

    draw_top_line(width, top_y, left_x, focused);

    top_y++; // the blocks are drawn below

    for (size_t i = 0; i < ui_block_vec.size(); i++) {
        struct ui_block uiblock = ui_block_vec[i];

        draw_ui_block(uiblock, uiblock.height, width, top_y + uiblock.top_y, left_x,
                      focused && focused_block_idx == i);
    }

    if (is_today()) draw_cursor(top_y, left_x + width, focused); // segfault
}

// private
void Day::draw_top_line(int width, int top_y, int left_x, bool focused) {
    std::string rel_str = get_relative_day();
    std::string day_str = get_day_str();
    std::string date_str = get_date_str();

    if (focused)
        attron(COLOR_PAIR(config_ptr->num({"ui", "colors", "focus"})));
    else if (rel_str != "")
        attron(COLOR_PAIR(config_ptr->num({"ui", "colors", "relative"})));

    if (rel_str == "") {
        int spacer_2 = width - (day_str.size() + date_str.size());
        int spacer_1 = width - date_str.size();

        if (spacer_2 > 0) {
            std::string spacer(spacer_2, ' ');
            mvprintw(top_y, left_x, "%s", (day_str + spacer + date_str).c_str());
        } else if (spacer_1 > 0) {
            mvprintw(top_y, left_x + spacer_1, "%s", date_str.c_str());
        } else {
            mvprintw(top_y, left_x, "%s", date_str.substr(0, width).c_str());
        }
    } else {
        int spacer_3 = width - (rel_str.size() + day_str.size() + 1 + date_str.size());
        int spacer_2 = width - (rel_str.size() + date_str.size());
        int spacer_1 = width - date_str.size();

        if (spacer_3 > 0) {
            std::string spacer(spacer_3, ' ');
            mvprintw(top_y, left_x, "%s", (rel_str + spacer + day_str + " " + date_str).c_str());
        } else if (spacer_2 > 0) {
            std::string spacer(spacer_2, ' ');
            mvprintw(top_y, left_x, "%s", (rel_str + spacer + date_str).c_str());
        } else if (spacer_1 > 0) {
            mvprintw(top_y, left_x + spacer_1, "%s", date_str.c_str());
        } else {
            mvprintw(top_y, left_x, "%s", date_str.substr(0, width).c_str());
        }
    }

    if (focused)
        attroff(COLOR_PAIR(config_ptr->num({"ui", "colors", "focus"})));
    else if (rel_str != "")
        attroff(COLOR_PAIR(config_ptr->num({"ui", "colors", "relative"})));
}

// private
void Day::draw_cursor(int top_y, int x_pos, bool focused) {
    float f_line = get_line_at_time(time(0)); // segfault
    int i_line = (int) f_line;
    int dec_3 = (int) (3 * (f_line - i_line));

    std::string character = "X";
    if      (dec_3 == 0) character = "🬂";
    else if (dec_3 == 1) character = "🬋";
    else if (dec_3 == 2) character = "🬭";

    if (focused) attron(COLOR_PAIR(config_ptr->num({"ui", "colors", "focus"})));
    else attron(COLOR_PAIR(config_ptr->num({"ui", "colors", "relative"})));

    mvprintw(top_y + i_line, x_pos, "%s", character.c_str());

    if (focused) attroff(COLOR_PAIR(config_ptr->num({"ui", "colors", "focus"})));
    else attroff(COLOR_PAIR(config_ptr->num({"ui", "colors", "relative"})));
}

// private
void Day::draw_ui_block(struct ui_block uiblock,
                        int height, int width, int top_y, int left_x, bool focused) {
    if (uiblock.block.get_collapsible()) {
        width -= 2;
        left_x++;
    }

    attron(COLOR_PAIR(uiblock.block.get_color()));
    custom_box(height, width, top_y, left_x,
               uiblock.block.get_important()? BOX_IMPORTANT : BOX_NORMAL, focused);
    attroff(COLOR_PAIR(uiblock.block.get_color()));

    if (focused) attron(COLOR_PAIR(config_ptr->num({"ui", "colors", "focus"})));

    std::string hour = uiblock.block.get_t_start_hour_str();
    mvprintw(top_y, left_x + 1, "%s", hour.c_str());

    // if there is not a block right below, specify the ending time
    if (!uiblock.bottom_adjacent) {
        hour = uiblock.block.get_t_end_hour_str();
        int draw_height = (height == 2)? 0 : height - 1;
        draw_height += top_y;

        mvprintw(draw_height, left_x + width - hour.size() - 1, "%s", hour.c_str());
    }

    draw_ui_block_title(uiblock, height - 2, left_x + 2, top_y + 1);

    if (focused) attroff(COLOR_PAIR(config_ptr->num({"ui", "colors", "focus"})));
}

// private
void Day::draw_ui_block_title(struct ui_block uiblock, int height, int left_x, int top_y) {
    std::vector<std::string> title_vec = uiblock.title_vec;

    if (height == 0) {
        if (uiblock.block.get_collapsible()) left_x--;

        mvprintw(top_y, left_x, "%s", title_vec[0].c_str());
    } else {
        int start_line = (height - title_vec.size()) / 2;
        if (title_vec.size() >= height) start_line = 0;

        for (int i = 0; i < title_vec.size(); i++) {
            if (start_line + i == height) break;

            mvprintw(top_y + start_line + i, left_x, "%s", title_vec[i].c_str());
        }
    }
}

// public
void Day::populate_vector() {
    std::vector<int> highlighted_ids = {};
    int focused_id = 0;

    if (ui_block_vec.size() != 0) {
        // find the id of the focused block(s)
        for (struct ui_block uiblock : ui_block_vec)
            if (uiblock.highlighted) highlighted_ids.push_back(uiblock.block.get_id());

        focused_id = ui_block_vec[focused_block_idx].block.get_id();
    }

    ui_block_vec.clear();

    int i = 0;
    for (Block block : database_ptr->get_blocks_on_day(date)) { i++;
        struct ui_block new_ui_block = { block, false, false, 0, 0, {} };

        // if this block's id is in the focused list
        if (std::find(highlighted_ids.begin(), highlighted_ids.end(), block.get_id())
            != highlighted_ids.end()) new_ui_block.highlighted = true;

        if (block.get_id() == focused_id) focused_block_idx = i;

        // sets the block before's bottom adjecency
        if (ui_block_vec.size() != 0) {
            struct ui_block &last = ui_block_vec.back();

            last.bottom_adjacent =
                last.block.get_time_t_start() + last.block.get_duration()
                == block.get_time_t_start();
        }

        ui_block_vec.push_back(new_ui_block);
    }

    if (focused_block_idx >= ui_block_vec.size())
        focused_block_idx = ui_block_vec.size() - 1;
}

// private
void Day::resize_width(int total_width) {
    if (total_width == last_width) return;
    last_width = total_width;

    int text_width = total_width - 4;

    for (struct ui_block& uiblock : ui_block_vec) {
        uiblock.title_vec.clear();

        std::string title = uiblock.block.get_title();

        float f_linecount = title.size();
        f_linecount /= text_width;
        int linecount = (int) std::ceil(f_linecount);

        for (int i = 0; i < linecount; i++)
            uiblock.title_vec.push_back(title.substr(i*text_width, text_width));
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
    time_t previous_end_time = get_date_time() + day_start;
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
        if (ui_block_vec.empty()) {
            start_line = 0;
            height = (float) last_height - 0.33333;
            start_time = previous_end_time;
            duration = day_end - day_start;
        } else {
            struct ui_block uiblock = ui_block_vec.back(); // segfault

            start_line = uiblock.top_y + uiblock.height - 0.33333;
            height = last_height - previous_end_line + 0.33333;
            start_time = previous_end_time;
            duration = std::mktime(&date) + day_end - previous_end_time;
        }
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
    std::cout << "highlighted: " << highlighted << std::endl;
    std::cout << "day start: " << day_start << std::endl;
    std::cout << "day end: " << day_end << std::endl;

    if (ui_block_vec.empty()) {
        std::cout << "no blocks in vector" << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << "LIST OF BLOCKS:" << std::endl;
    for (struct ui_block uiblock : ui_block_vec) {
        std::cout << std::endl;
        std::cout << "top_y: " << uiblock.top_y << ", height: " << uiblock.height << ", highlighted: " << uiblock.highlighted << std::endl;
        uiblock.block.dump_info();
    }
    std::cout << std::endl << "END OF BLOCKS" << std::endl;
}

// public
std::string Day::get_date_str() const {
    char buffer[20];
    struct tm copy = date;
    std::strftime(buffer, sizeof(buffer), date_format.c_str(), &copy);

    return std::string(buffer);
}

// public
std::string Day::get_day_str() const {
    char buffer[20];
    struct tm copy = date;
    std::strftime(buffer, sizeof(buffer), day_format.c_str(), &copy);

    return std::string(buffer);
}

// public
bool Day::has_blocks() { return !ui_block_vec.empty(); }

// public
void Day::move_focus(int distance) {
    focused_block_idx += distance;
    set_focus_inbounds();
}

// public
void Day::set_focus(int new_focus) {
    focused_block_idx = new_focus;
    set_focus_inbounds();
}

// public
int Day::get_focus() { return focused_block_idx; }

// private
void Day::set_focus_inbounds() {
    if (focused_block_idx < 0)
        focused_block_idx = 0;

    if (focused_block_idx >= ui_block_vec.size())
        focused_block_idx = ui_block_vec.size() - 1;
}

// public
void Day::set_highlighted(bool new_highlighted) { highlighted = new_highlighted; }

// public
void Day::integrity_check() const {
    int previous_end_line = 0;
    for (struct ui_block uiblock : ui_block_vec) {
        if (uiblock.top_y < previous_end_line) {
            uiblock.block.dump_info();
            throw std::runtime_error(error_str + "block " + uiblock.block.get_title()
                                     + " overlaps with previous block");
        }

        previous_end_line = uiblock.top_y + uiblock.height;
    }

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

    if (date_t == today_t - day)
        return config_ptr->str({"ui", "relative_time", "yesterday"});
    else if (date_t == today_t)
        return config_ptr->str({"ui", "relative_time", "today"});
    else if (date_t == today_t + day)
        return config_ptr->str({"ui", "relative_time", "tomorrow"});
    else if (date_t == today_t + 7*day)
        return config_ptr->str({"ui", "relative_time", "next_week"});
    else if (date_t == today_t - 7*day)
        return config_ptr->str({"ui", "relative_time", "last_week"});

    else if (date.tm_mday == today.tm_mday && date.tm_year == today.tm_year) {
        if (date.tm_mon == today.tm_mon + 1)
            return config_ptr->str({"ui", "relative_time", "next_month"});
        else if (date.tm_mon == today.tm_mon - 1)
            return config_ptr->str({"ui", "relative_time", "last_month"});
        else return "";
    } else if (date.tm_mday == today.tm_mday && date.tm_mon == today.tm_mon) {
        if (date.tm_year == today.tm_year + 1)
            return config_ptr->str({"ui", "relative_time", "next_year"});
        else if (date.tm_year == today.tm_year - 1)
            return config_ptr->str({"ui", "relative_time", "last_year"});
        else return "";
    } else return "";
}

// private
void Day::custom_box(int height, int width, int top_y, int left_x, en_box_type type, bool filled) {
    std::string type_id;

    if (type == BOX_NORMAL) type_id = "normal";
    else if (type == BOX_IMPORTANT) type_id = "important";
    else if (type == BOX_BACKGROUND) type_id = "background";

    std::string tl, tr, bl, br, hz, vr, fill;
    tl = config_ptr->str({"ui", "boxdrawing", type_id+"_tl"});
    tr = config_ptr->str({"ui", "boxdrawing", type_id+"_tr"});
    bl = config_ptr->str({"ui", "boxdrawing", type_id+"_bl"});
    br = config_ptr->str({"ui", "boxdrawing", type_id+"_br"});
    hz = config_ptr->str({"ui", "boxdrawing", type_id+"_hz"});
    vr = config_ptr->str({"ui", "boxdrawing", type_id+"_vr"});
    fill = filled ? config_ptr->str({"ui", "boxdrawing", "highlight_fill"})
                  : config_ptr->str({"ui", "boxdrawing", type_id+"_fill"});

    std::string horizontal = "";
    for (int i = 0; i < width - 2; i++) horizontal += hz;
    std::string top = tl + horizontal + tr;
    std::string bottom = bl + horizontal + br;

    mvprintw(top_y, left_x, "%s", top.c_str());

    for (int i = 1; i < height - 1; i++) {
        std::string line = vr;
        for (int j = 1; j < width - 1; j++) line += fill;
        line += vr;

        mvprintw(top_y + i, left_x, "%s", line.c_str());
    }

    mvprintw(top_y + height - 1, left_x, "%s", bottom.c_str());
}

// public
int Day::get_focus_line() {
    if (ui_block_vec.empty()) return 0;

    return ui_block_vec[focused_block_idx].top_y
         + ui_block_vec[focused_block_idx].height / 2;
}

// public
void Day::set_focus_line(int line) {
    if (ui_block_vec.empty()) {
        focused_block_idx = 0;
        return;
    }

    // check if task is above first task or after last task
    if (line < ui_block_vec.front().top_y) {
        focused_block_idx = 0;
        return;
    } else if (line > ui_block_vec.back().top_y) {
        focused_block_idx = ui_block_vec.size() - 1;
        return;
    }

    int previous_end_line = 0;

    for (size_t i = 0; i < ui_block_vec.size(); i++) {
        struct ui_block uiblock = ui_block_vec[i];

        // check if the line is in this block
        if (line >= uiblock.top_y && line <= uiblock.top_y + uiblock.height - 1) {
            focused_block_idx = i;
        // check if line is in the gap above this block
        } else if (line >= previous_end_line && line < uiblock.top_y) {
            int gap_height = uiblock.top_y - previous_end_line; // amount of lines
            int gap_line = line - previous_end_line; // starting at 0

            float f_gap_pos = gap_line + 1; // so that middle line is treated downwards
            f_gap_pos /= gap_height;

            if (f_gap_pos <= 0.5 + 0.00001) {
                focused_block_idx = i - 1;
            } else {
                focused_block_idx = i;
            }
        } else {
            previous_end_line = uiblock.top_y + uiblock.height;
            continue;
        }

        return;
    }
}

// public
Block Day::get_focused_block() { return ui_block_vec[focused_block_idx].block; }

// public
int Day::get_focus_time_start() {
    return ui_block_vec[focused_block_idx].block.get_time_t_start();
}

// public
bool Day::is_date_equal(struct tm other) const {
    struct tm cp = date;
    return std::mktime(&cp) == std::mktime(&other);
}

// public
time_t Day::get_date_time() const {
    struct tm cp = date;
    return std::mktime(&cp);
}

// public
struct tm Day::get_date() const { return date; }
bool Day::get_highlighted() const { return highlighted; }

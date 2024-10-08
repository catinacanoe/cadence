#include "Ui.h"

Ui::Ui(std::vector<std::string> args_)
:   args(args_),
    config("/home/canoe/.config/cadence/conf.toml"),
    database(&config),
    week(&database, &config)
{
    current_mode = MD_WEEK;
    key_sequence = "";
    time_since_last_key = 0;
}

//public
void Ui::main() {
    if (args.size() == 1) {
        init_ncurses();

        while (true) if (draw_cycle()) break;

        endwin();

        week.dump_info();
    } else {
        for (std::string str : args) {
            std::cout << str << std::endl;
        }
    }
}

// private
void Ui::init_ncurses() {
    setlocale(LC_ALL, ""); // use system default locale
    initscr(); // sets up mem and clears screen
    curs_set(0); // hides cursor
    cbreak(); // ^C exits program
    nodelay(stdscr, true); // getch() is non blocking now
    keypad(stdscr, true); // detect special keys like arrows
    noecho(); // don't write what you type to the screen

    if (!has_colors()) throw std::runtime_error("terminal doesn't support colors");
    else start_color();

    use_default_colors();

    // default background
    init_pair(0, -1, -1);
    for (int i = 1; i < 8; i++) init_pair(i,  i, -1);
    
    // black background
    init_pair(8, -1, 8);
    for (int i = 1; i < 8; i++) init_pair(i+8, i, 8);

    // black foreground, colored background
    init_pair(16, 0, 15);
    for (int i = 1; i < 8; i++) init_pair(i+16, 0, i);
}

// private
bool Ui::draw_cycle() {
    wclear(stdscr);

    int height, width; getmaxyx(stdscr, height, width);

    week.draw(height - 1, width, 0, 0);
    draw_bottom_bar(height, width);

    wrefresh(stdscr);

    char key = getch();

    if (key == ERR) {
        if (time_since_last_key > config.num({"keybinds", "timeout"})
         && current_mode != MD_WEEK_RENAME) { // we are typing for long in week_rename so no timeout
            key_sequence = "";
        }

        napms(config.num({"ui", "frame_time"}));
        time_since_last_key += config.num({"ui", "frame_time"});
        return false;
    }

    time_since_last_key = 0;

    switch (key) { // update key_sequence with appropriate escape, or just the char
        case 4:  key_sequence += "<left>";  break;
        case 3:  key_sequence += "<up>";    break;
        case 2:  key_sequence += "<down>";  break;
        case 5:  key_sequence += "<right>"; break;
        case 7:  key_sequence += "<bs>";    break;
        case 10: key_sequence += "<cr>";    break;
        case 9:  key_sequence += "<tab>";   break;
        case 27: key_sequence += "<esc>";   break;
        case 18: key_sequence += "<c-r>";   break;
        default: key_sequence += key;       break;
    }

    std::string sequence = key_sequence;
    key_sequence = "";

    switch(current_mode) {
        case MD_WEEK:

            if (sequence == config.str({"keybinds", "quit"}))
                return true;

            else if (sequence == config.str({"keybinds", "left"}))
                week.move_focus(-1);
            else if (sequence == config.str({"keybinds", "up"}))
                week.move_block_focus(-1);
            else if (sequence == config.str({"keybinds", "down"}))
                week.move_block_focus(1);
            else if (sequence == config.str({"keybinds", "right"}))
                week.move_focus(1);

            else if (sequence == config.str({"keybinds", "week", "rename"})) {
                if (week.block_focused()) {
                    current_mode = MD_WEEK_RENAME;
                    // key_sequence = week.get_focused_block().get_title();
                }
            }

            else if (sequence == config.str({"keybinds", "week", "new_block_below"})) {
                if (week.new_block_below()) { // returns success state
                    current_mode = MD_WEEK_RENAME;
                }
            }
            else if (sequence == config.str({"keybinds", "week", "new_block_above"})) {
                if (week.new_block_above()) { // returns success state
                    current_mode = MD_WEEK_RENAME;
                }
            }

            else if (sequence == config.str({"keybinds", "week", "move_up"}))
                week.move_block_up();
            else if (sequence == config.str({"keybinds", "week", "move_down"}))
                week.move_block_down();
            else if (sequence == config.str({"keybinds", "week", "move_right"}))
                week.move_block_right();
            else if (sequence == config.str({"keybinds", "week", "move_left"}))
                week.move_block_left();

            else if (sequence == config.str({"keybinds", "week", "extend_top_up"}))
                week.extend_top_up();
            else if (sequence == config.str({"keybinds", "week", "extend_top_down"}))
                week.extend_top_down();
            else if (sequence == config.str({"keybinds", "week", "extend_bottom_up"}))
                week.extend_bottom_up();
            else if (sequence == config.str({"keybinds", "week", "extend_bottom_down"}))
                week.extend_bottom_down();

            else if (sequence == config.str({"keybinds", "week", "move_up_snap"}))
                while(week.move_block_up()) {}
            else if (sequence == config.str({"keybinds", "week", "move_down_snap"}))
                while(week.move_block_down()) {}
            else if (sequence == config.str({"keybinds", "week", "extend_top_up_snap"}))
                while(week.extend_top_up()) {}
            else if (sequence == config.str({"keybinds", "week", "extend_bottom_down_snap"}))
                while(week.extend_bottom_down()) {}

            else if (sequence == config.str({"keybinds", "week", "set_col_white"}))
                week.set_block_color("white");
            else if (sequence == config.str({"keybinds", "week", "set_col_red"}))
                week.set_block_color("red");
            else if (sequence == config.str({"keybinds", "week", "set_col_green"}))
                week.set_block_color("green");
            else if (sequence == config.str({"keybinds", "week", "set_col_yellow"}))
                week.set_block_color("yellow");
            else if (sequence == config.str({"keybinds", "week", "set_col_blue"}))
                week.set_block_color("blue");
            else if (sequence == config.str({"keybinds", "week", "set_col_purple"}))
                week.set_block_color("purple");
            else if (sequence == config.str({"keybinds", "week", "set_col_aqua"}))
                week.set_block_color("aqua");
            else if (sequence == config.str({"keybinds", "week", "set_col_gray"}))
                week.set_block_color("gray");

            else if (sequence == config.str({"keybinds", "week", "toggle_important"}))
                week.block_toggle_important();
            else if (sequence == config.str({"keybinds", "week", "toggle_collapsible"}))
                week.block_toggle_collapsible();

            else if (sequence == config.str({"keybinds", "week", "edit_block_source"}))
                week.edit_block_source();
            else if (sequence == config.str({"keybinds", "week", "follow_link"}))
                week.follow_link();

            else if (sequence == config.str({"keybinds", "week", "copy_left"}))
                week.copy_block_lateral(-1);
            else if (sequence == config.str({"keybinds", "week", "copy_right"}))
                week.copy_block_lateral(1);
            else if (sequence == config.str({"keybinds", "week", "copy_down"}))
                week.copy_block_vertical(true);
            else if (sequence == config.str({"keybinds", "week", "copy_up"}))
                week.copy_block_vertical(false);

            else if (sequence == config.str({"keybinds", "week", "undo"}))
                week.undo();
            else if (sequence == config.str({"keybinds", "week", "redo"}))
                week.redo();

            else if (sequence == config.str({"keybinds", "week", "remove"}))
                week.remove_block();

            else if (sequence == config.str({"keybinds", "week", "reload"}))
                week.reload_all();

            else key_sequence = sequence;

            break;
        case MD_WEEK_RENAME:

            size_t confirm_pos = sequence.find(config.str({"keybinds", "week", "confirm_rename"}));
            size_t cancel_pos = sequence.find(config.str({"keybinds", "week", "cancel_rename"}));
            size_t clear_pos = sequence.find(config.str({"keybinds", "week", "clear_rename"}));
            size_t bs_pos = sequence.find("<bs>");

            if (confirm_pos != std::string::npos) {
                week.rename_block(sequence.substr(0, confirm_pos));
                current_mode = MD_WEEK;
            }
            else if (cancel_pos != std::string::npos)
                current_mode = MD_WEEK;
            else if (clear_pos != std::string::npos)
                key_sequence = "";
            else if (bs_pos != std::string::npos) {
                if (bs_pos > 0) key_sequence = sequence.substr(0, bs_pos - 1);
                else key_sequence = "";
            }
            else key_sequence = sequence;

            break;
    }

    return false;
}

void Ui::draw_bottom_bar(int height, int width) {
    std::string str_status, str_link, str_keys;

    str_status = "";
    int col_status = 0;

    switch(current_mode) {
        case MD_WEEK:
            str_status = " NORMAL ";
            col_status = config.num({"ui", "colors", "status_normal"});
            break;
        case MD_WEEK_RENAME:
            str_status = " RENAME ";
            col_status = config.num({"ui", "colors", "status_rename"});
            break;
    }

    str_keys = " " + key_sequence;

    str_link = week.get_current_link();
    if (!str_link.empty()) str_link = " "+str_link+" ";

    str_status = str_status.substr(0, width);
    str_keys   = str_keys  .substr(0, width);
    str_link   = str_link  .substr(0, width);

    int col_keys = 8;
    int col_link = week.get_current_link_col();

    attron(A_BOLD);

    // fill the bottom with black color
    attron(COLOR_PAIR(col_keys));
    std::string spaces(width, ' ');
    mvprintw(height - 1, 0, "%s", spaces.c_str());
    attroff(COLOR_PAIR(col_keys));

    // print the current link
    attron(COLOR_PAIR(col_link));
    mvprintw(height - 1, width - str_link.size(), "%s", str_link.c_str());
    attroff(COLOR_PAIR(col_link));

    // print key sequence
    attron(COLOR_PAIR(col_keys));
    mvprintw(height - 1, str_status.size(), "%s", str_keys.c_str());
    attroff(COLOR_PAIR(col_keys));

    // write status
    attron(COLOR_PAIR(col_status));
    mvprintw(height - 1, 0, "%s", str_status.c_str());
    attroff(COLOR_PAIR(col_status));

    attroff(A_BOLD);
}

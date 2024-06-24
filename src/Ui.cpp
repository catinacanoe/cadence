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
         && current_mode != MD_WEEK_RENAME) {
            key_sequence = "";
        }

        napms(config.num({"ui", "frame_time"}));
        time_since_last_key += config.num({"ui", "frame_time"});
        return false;
    }

    time_since_last_key = 0;

    switch (key) {
        case 4:  key_sequence += "<left>";  break;
        case 3:  key_sequence += "<up>";    break;
        case 2:  key_sequence += "<down>";  break;
        case 5:  key_sequence += "<right>"; break;
        case 7:  key_sequence += "<bs>";    break;
        case 10: key_sequence += "<cr>";    break;
        case 9:  key_sequence += "<tab>";   break;
        case 27: key_sequence += "<esc>";   break;
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
                if (week.new_block_below()) {
                    current_mode = MD_WEEK_RENAME;
                    // key_sequence = week.get_focused_block().get_title();
                }
            }

            else if (sequence == config.str({"keybinds", "week", "undo"}))
                week.undo();
            else if (sequence == config.str({"keybinds", "week", "redo"}))
                week.redo();

            else if (sequence == config.str({"keybinds", "week", "remove"}))
                week.remove_block();

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

// TODO eventually make this similar to Day::top_bar to handle small widths
void Ui::draw_bottom_bar(int height, int width) {
    std::string status = "";
    int status_col = 0;

    switch(current_mode) {
        case MD_WEEK:        status = " NORMAL "; status_col = 7+16; break;
        case MD_WEEK_RENAME: status = " RENAME "; status_col = 5+16; break;
    }

    attron(A_BOLD);

    attron(COLOR_PAIR(8));
    std::string spaces(width, ' ');
    mvprintw(height - 1, 0, "%s", spaces.c_str());
    attroff(COLOR_PAIR(8));

    attron(COLOR_PAIR(status_col));
    mvprintw(height - 1, 0, "%s", status.c_str());
    attroff(COLOR_PAIR(status_col));

    attron(COLOR_PAIR(8));
    mvprintw(height - 1, status.size() + 1, "%s", key_sequence.c_str());
    attroff(COLOR_PAIR(8));

    attroff(A_BOLD);
}

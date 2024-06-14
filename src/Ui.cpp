#include "Ui.h"

Ui::Ui() {
    database = Database((std::filesystem::path) "/home/canoe/repos/cadence/run/");

    time_t day_start = 6 * 60 * 60; // these two will be sourced from config file in the future
    time_t day_end = 22 * 60 * 60;

    week = Week(&database, 20, 6, day_start, day_end);
}

// private
void Ui::init_ncurses() {
    setlocale(LC_ALL, ""); // use system default locale
    initscr(); // sets up mem and clears screen
    curs_set(0); // hides cursor
    cbreak(); // ^C exits program
    noecho(); // don't write what you type to the screen

    if (!has_colors()) throw std::runtime_error("terminal doesn't support colors");
    else start_color();

    use_default_colors();

    init_pair(0, -1, -1); // white
    init_pair(15,  0, -1); // black
    for (int i = 1; i < 8; i++) init_pair(i,  i, -1); // all 7 colors
}

// private
bool Ui::draw_cycle() {
    clear();
    refresh();

    int height, width; getmaxyx(stdscr, height, width);

    week.draw(height, width, 0, 0);

    return (getch() == ';');
}

//public
void Ui::main(std::vector<std::string> args) {
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

#include "Block.h"
#include "Database.h"
#include "Week.h"

#include <iostream>
#include <ncurses.h>
#include <memory>
#include <vector>


int main(int argc, char** argv) {
    initscr(); // sets up mem and clears screen
    curs_set(0); // hides cursor
    cbreak(); // ^C exits program
    noecho(); // don't write what you type to the screen

    if (!has_colors()) {
        printw("Terminal doesn't support colors");
        getch();
        return -1;
    } else {
        start_color();
    }

    use_default_colors();

    init_pair(0, -1, -1); // white
    init_pair(1,  1, -1); // red
    init_pair(2,  2, -1); // green
    init_pair(3,  3, -1); // yellow
    init_pair(4,  4, -1); // blue
    init_pair(5,  5, -1); // purple
    init_pair(6,  6, -1); // aqua
    init_pair(7,  7, -1); // gray
    init_pair(15,  0, -1); // black
    
    Database database = Database((std::filesystem::path) "/home/canoe/repos/cadence/run/");
     
    time_t day_start = 6 * 60 * 60; // these two will be sourced from config file in the future
    time_t day_end = 22 * 60 * 60;

    Week week = Week(&database, 20, 6, day_start, day_end);

    int height, width;
    getmaxyx(stdscr, height, width);
    week.draw(height, width, 0, 0);

    refresh();

    getch();

    endwin();

    week.dump_info();

    std::cout << height << std::endl;
    std::cout << width << std::endl;
}

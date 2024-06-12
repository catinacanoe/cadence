#include "Block.h"
#include "Database.h"
#include "Week.h"

#include <iostream>
#include <ncurses.h>
#include <memory>
#include <vector>

time_t day_start = 6 * 60 * 60; // these two will be sourced from config file in the future
time_t day_end = 22 * 60 * 60;

int main(int argc, char** argv) {
    Database database = Database((std::filesystem::path) "/home/canoe/repos/cadence/run/");
    database.dump_info();
     
    Week week = Week(&database, today, 19, 5);

    return 0;

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
    printw("lolll");
    
    getch();

    endwin();
}

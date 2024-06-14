#pragma once

#include "Database.h"
#include "Week.h"

#include <ncursesw/ncurses.h>

class Ui {
private:
    Database database;
    Week week;
    // Config
    void init_ncurses();
    bool draw_cycle(); // returns true if program should exit
public:
    Ui();

    // static void double_box(WINDOW *win);

    void main(std::vector<std::string> args);
};

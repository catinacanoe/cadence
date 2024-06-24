#pragma once

#include "Database.h"
#include "Week.h"

// #include "include/curses.h"
#include <ncursesw/ncurses.h>

class Ui {
public:
    Ui(std::vector<std::string> args_);
    void main();
private:
    std::vector<std::string> args;
    Config config;
    Database database;
    Week week;

    enum en_mode { MD_WEEK, MD_WEEK_RENAME };
    en_mode current_mode;
    std::string key_sequence;
    int time_since_last_key;

    void init_ncurses();
    bool draw_cycle(); // returns true if program should exit
    void draw_bottom_bar(int height, int width);
};

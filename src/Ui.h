#pragma once

#include "Database.h"
#include "Week.h"

#include <ncursesw/ncurses.h>

class Ui {
private:
    Database database;
    Week week;
    Config config;

    std::vector<std::string> args;

    enum en_mode { MD_WEEK, MD_WEEK_RENAME };
    en_mode current_mode;
    std::string key_sequence;
    int time_since_last_key;

    void init_ncurses();
    bool draw_cycle(); // returns true if program should exit
    void draw_bottom_bar(int height, int width);
public:
    Ui(std::vector<std::string> args_);

    void main();
};

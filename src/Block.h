#pragma once
#include <vector>
#include <filesystem>
#include <boost/algorithm/string/trim.hpp>
#include <ctime>
#include <fstream>
#include <iostream>

class Block {
private:
    std::string title, link;
    int id, color;
    bool collapsible;
    std::vector<bool> recur_days;
    struct tm* t_start, t_end;
public:
    Block(std::filesystem::path savefile);
    bool is_on_range(struct tm* start, struct tm* end);
    std::string dump_info();
};

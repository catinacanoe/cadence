#pragma once

#include "../lib/toml.hpp"
#include <filesystem>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>

class Config {
private:
    toml::table toml_table;
    std::string error_str;
public:
    Config(std::filesystem::path config_file); // loads config from toml file
    // Config();
    
    std::string str(std::vector<std::string> keys);
    int num(std::vector<std::string> keys);
    
    void dump_info();
};

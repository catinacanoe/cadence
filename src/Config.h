#pragma once

// #include "toml.hpp"
// #include "../lib/toml.hpp"
#include <toml++/toml.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

class Config {
public:
    Config(std::filesystem::path config_file); // loads config from toml file
    
    std::string str(std::vector<std::string> keys);
    int num(std::vector<std::string> keys);
    
    void dump_info();
private:
    toml::table toml_table;
    std::string error_str;
};

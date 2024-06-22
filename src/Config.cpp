#include "Config.h"

// public
Config::Config(std::filesystem::path config_file) {
    error_str = "error in config file: " + config_file.string();

    if (!std::filesystem::is_regular_file(config_file))
        throw std::runtime_error(error_str + ", file does not exist");

    try {
        toml_table = toml::parse_file(config_file.string());
    } catch (const toml::parse_error& err) {
        std::cerr << error_str << ", error parsing:\n" << err << std::endl;
        throw std::runtime_error("^");
    }
}

// Config::Config() {
//     error_str = "";
//     toml_table = toml::table();
// }

// public
void Config::dump_info() {
    std::cout << str({"keybinds", "default", "left"}) << std::endl;
}

// public
std::string Config::str(std::vector<std::string> keys) {
    if (keys.empty()) return "";

    toml::node_view current_node = toml_table[keys[0]];
    for (size_t i = 1; i < keys.size(); i++) current_node = current_node[keys[i]];

    return current_node.value_or("");
}

//public
int Config::num(std::vector<std::string> keys) {
    if (keys.empty()) return 0;

    toml::node_view current_node = toml_table[keys[0]];
    for (size_t i = 1; i < keys.size(); i++) current_node = current_node[keys[i]];

    return current_node.value_or(0);
}

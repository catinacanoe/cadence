#include "Block.h"

Block::Block(std::filesystem::path savefile) {
    std::ifstream file;
    file.open(savefile);
    if (!file.is_open()) throw std::runtime_error("unable to open block save file: " + savefile.string());

    std::string error;
    int linenum;
    while (file) {
        linenum++;
        error = savefile.string()+" @ line "+std::to_string(linenum)+", ";
        
        std::string line;
        std::getline(file, line);
        boost::algorithm::trim(line);
        if (line == "") continue;

        std::cout << line << std::endl;

        // size_t field_value_sep = line.find("=");
        // if (field_value_sep == -1) {
        //     error += "failed to parse: '"+line+"'";
        //     throw runtime_error(error);
        // }

        // string field = line.substr(0, field_value_sep);
        // string value = line.substr(field_value_sep+1);
        // boost::algorithm::trim(field);
        // boost::algorithm::trim(value);

        // options[field] = value;
    }

    file.close();
}

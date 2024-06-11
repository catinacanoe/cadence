#include "Block.h"

Block::Block(std::filesystem::path savefile) {
    std::ifstream file;
    file.open(savefile);

    if (!file.is_open())
        throw std::runtime_error("unable to open block save file: " + savefile.string());

    init_fields();
    source_file = savefile;
    parse_filename(savefile.stem());

    en_parsing_block current_block = BLK_NA;
    bool parsed_time_blk = false, parsed_meta_blk = false;

    std::string current_line;
    int linenum = 0;

    while (file) {
        std::getline(file, current_line); linenum++;

        boost::algorithm::trim(current_line);
        if (current_line == "") continue;

        parse_line(current_line, linenum, current_block, parsed_time_blk, parsed_meta_blk);
    }

    file.close();

    integrity_check();
}

void Block::integrity_check() {
    std::string err = "in file '" + source_file.string() + "', ";

    if (t_start.tm_year == 0) throw std::runtime_error(err+"uninitialized start time");
    if (t_end.tm_year == 0) throw std::runtime_error(err+"uninitialized end time");

    if (std::mktime(&t_start) >= std::mktime(&t_end))
        throw std::runtime_error(err+"start time ("+get_t_start_str()
                                 +") is after end time ("+get_t_end_str()+")");

    if (title == "") throw std::runtime_error(err+"title is empty");

    if (id < 0 || id > 99999) throw std::runtime_error
        (err+"id is out of range [0, 99999] ("+std::to_string(id)+")");

    if (group < 0 || group > 99999) throw std::runtime_error
        (err+"group is out of range [0, 99999] ("+std::to_string(group)+")");

    if (color < 0 || color > 7) throw std::runtime_error
        (err+"color is out of range [0, 7] ("+std::to_string(color)+")");
}

void Block::parse_line(std::string line,
                       int line_num,
                       en_parsing_block& cur_block,
                       bool& parsed_time,
                       bool& parsed_meta) {
    std::string error = "error parsing " + source_file.string()
                        + " @ line " + std::to_string(line_num);

    switch (cur_block) {
        case BLK_NA: // look for the start of a new block
            if (line == "@document.meta") cur_block = BLK_META;
            else if (line == "@code lua time") cur_block = BLK_TIME;
            break;
        case BLK_META:
            parse_field_line(line, ":", error, parsed_meta, cur_block);
            break;
        case BLK_TIME:
            parse_field_line(line, "=", error, parsed_time, cur_block);
            break;
    }
}

void Block::parse_field_line(std::string line,
                             std::string delimiter,
                             std::string error,
                             bool& cur_block_parsed,
                             en_parsing_block& cur_block) {
    if (line == "@end") { cur_block = BLK_NA; cur_block_parsed = true; return; }
    if (cur_block_parsed)
        throw std::runtime_error(error + ", a block is defined twice");

    size_t delim_pos = line.find(delimiter);
    if (delim_pos == std::string::npos)
        throw std::runtime_error(error + ", could not find '"+delimiter+"' delimiter");

    init_field(line.substr(0, delim_pos), line.substr(delim_pos + 1), error);
}

void Block::init_field(std::string name, std::string contents, std::string error) {
    boost::algorithm::trim(name);
    boost::algorithm::trim(contents);

    // identify specific field and parse contents accordingly
    if (name == "collapsible") {
        collapsible = true;
    } else if (name == "important") {
        important = true;
    } else if (name == "link") {
        link = contents;
        if (contents == "") {
            link_type = LINK_NA;
        } else if (contents.find("http://") == 0 || contents.find("https://") == 0) {
            link_type = LINK_HTTP;
        } else {
            try {
                std::stoi(contents);
                link_type = LINK_TASK;
            } catch (const std::exception& e) {
                link_type = LINK_FILE;
            }
        }
    } else if (name == "start") {
        strptime(contents.c_str(), date_format, &t_start);
    } else if (name == "end") {
        strptime(contents.c_str(), date_format, &t_end);
    } else if (name == "group") {
        try {
            group = std::stoi(contents);
        } catch (const std::exception& e) {
            throw std::runtime_error(error + ", can't parse group id into integer");
        }
    } else if (name == "color") {
        color = -1;
        for (size_t i = 0; i < 8; i++) {
            if (contents == color_names[i]) color = i;
        }

        if (color == -1) { try {
            color = std::stoi(contents);
        } catch (const std::exception& e) {
            throw std::runtime_error(error
                  + ", cant parse color '"+contents+"' into integer");
        }}
    } else {
        throw std::runtime_error(error + ", unrecognized field name: " + name);
    }
}

void Block::parse_filename(std::string filename) {
    size_t period_pos = filename.find(".");

    if (period_pos == std::string::npos) throw std::runtime_error(
        "failed to parse filename '" + filename
        + "', there should be a '.' between the title and id");

    title = filename.substr(0, period_pos);

    try {
        id = std::stoi(filename.substr(period_pos + 1));
    } catch (const std::exception& e) {
        throw std::runtime_error("failed to parse id from filename '" + filename + "'");
    }
}

void Block::init_fields() {
    title = "";
    link = "";
    link_type = LINK_NA;
    color = 0;
    id = 0;
    group = 0;
    collapsible = false;
    important = false;
    t_start = {0};
    t_end = {0};
    std::fill(modified, modified + field_count, false);
}

void Block::dump_info() {
    std::cout << "title: " << title << std::endl;
    std::cout << "id: " << id << std::endl;
    std::cout << "link: " << link << std::endl;
    std::cout << "link_type: " << link_type << std::endl;
    std::cout << "color: " << color << std::endl;
    std::cout << "group: " << group << std::endl;
    std::cout << "collapsible: " << collapsible << std::endl;
    std::cout << "important: " << important << std::endl;
    std::cout << "start: " << get_t_start_str() << std::endl;
    std::cout << "end: " << get_t_end_str() << std::endl;
    std::cout << "file: " << source_file.string() << std::endl;
}

void Block::save_to_file() {
    if (modified[FLD_TITLE]) {
        std::filesystem::path new_file = source_file.parent_path().string() + "/" + title + "." + std::to_string(id) + ".norg";
        std::rename(source_file.string().c_str(), new_file.string().c_str());
        source_file = new_file;

        modified[FLD_TITLE] = false;
    }

    std::vector<std::string> file_vec;
    bool writing_to_file = modified[FLD_LINK] ||
                           modified[FLD_COLOR] ||
                           modified[FLD_COLLAPSIBLE] ||
                           modified[FLD_IMPORTANT] ||
                           modified[FLD_START] ||
                           modified[FLD_END];

    if (writing_to_file) {
        std::ifstream infile;
        infile.open(source_file);
        if (!infile.is_open())
            throw std::runtime_error("unable to read block save file: " + source_file.string());

        std::string current_line;

        while (infile) {
            std::getline(infile, current_line);
            file_vec.push_back(current_line);
        } infile.close();
    }

    if (modified[FLD_LINK] ||
        modified[FLD_COLOR] ||
        modified[FLD_COLLAPSIBLE] ||
        modified[FLD_IMPORTANT]) {

        bool in_block = false;
        std::string line;

        for (size_t i = 0; i < file_vec.size(); i++) {
            line = file_vec[i];
            boost::algorithm::trim(line);
            if (line == "") continue;

            if (!in_block && line == "@document.meta") in_block = true;
            else if (in_block) {
                if (line == "@end") {
                    file_vec[i] = "";

                    if (modified[FLD_COLOR])
                        file_vec[i] += "color: " + get_color_str() + "\n";
                    if (modified[FLD_LINK] && link != "")
                        file_vec[i] += "link: " + link + "\n";
                    if (modified[FLD_COLLAPSIBLE] && collapsible)
                        file_vec[i] += "collapsible:\n";
                    if (modified[FLD_IMPORTANT] && important)
                        file_vec[i] += "important:\n";

                    file_vec[i] += "@end";
                    break;
                }

                if ((line.find("color:")       == 0 && modified[FLD_COLOR]) ||
                    (line.find("link:")        == 0 && modified[FLD_LINK]) ||
                    (line.find("collapsible:") == 0 && modified[FLD_COLLAPSIBLE]) ||
                    (line.find("important:")   == 0 && modified[FLD_IMPORTANT])) {

                    file_vec.erase(file_vec.begin() + i);
                    i--;
                }
            }
        }
    }

    if (modified[FLD_START] || modified[FLD_END]) {
        bool in_block = false;
        std::string line;

        for (size_t i = 0; i < file_vec.size(); i++) {
            line = file_vec[i];
            boost::algorithm::trim(line);
            if (line == "") continue;

            if (!in_block && line == "@code lua time") in_block = true;
            else if (in_block) {
                if (line == "@end") {
                    file_vec[i] = "";

                    if (modified[FLD_START])
                        file_vec[i] += "start = " + get_t_start_str() + "\n";
                    if (modified[FLD_END])
                        file_vec[i] += "end   = " + get_t_end_str() + "\n";

                    file_vec[i] += "@end";
                    break;
                }

                if ((line.find("start") == 0 && modified[FLD_START]) ||
                    (line.find("end")   == 0 && modified[FLD_END])) {

                    file_vec.erase(file_vec.begin() + i);
                    i--;
                }
            }
        }
    }

    if (writing_to_file) {
        std::ofstream outfile;
        outfile.open(source_file);
        if (!outfile.is_open())
            throw std::runtime_error("unable to write to block save file: " + source_file.string());

        for (size_t i = 0; i < file_vec.size() - 1; i++) {
            outfile << file_vec[i] << std::endl;
        } outfile << file_vec[file_vec.size() - 1];
        outfile.close();

        modified[FLD_LINK] =
        modified[FLD_COLOR] =
        modified[FLD_COLLAPSIBLE] =
        modified[FLD_IMPORTANT] =
        modified[FLD_START] =
        modified[FLD_END] = false;
    }
}

std::string Block::get_t_end_str() {
    strftime(date_buffer, sizeof(date_buffer), date_format, &t_end);
    return std::string(date_buffer);
}

std::string Block::get_t_start_str() {
    strftime(date_buffer, sizeof(date_buffer), date_format, &t_start);
    return std::string(date_buffer);
}

std::string Block::get_color_str() {
    return color_names[color];
}

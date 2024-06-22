#include "Block.h"

Block::Block(std::filesystem::path savefile, Config* cfg_ptr) {
    source_file_integrity(savefile);

    std::ifstream file;
    file.open(savefile);

    if (!file.is_open())
        throw std::runtime_error("unable to open block save file: " + savefile.string());

    source_file = savefile;
    init_fields();
    parse_filename(savefile.stem());

    en_parsing_block current_block = BLK_NA;
    bool parsed_time_blk = false, parsed_meta_blk = false;

    std::string current_line;
    int linenum = 0;

    hour_format = cfg_ptr->str({"ui", "date_formats", "hour_format"});
    parse_format = cfg_ptr->str({"ui", "date_formats", "parse_format"});

    while (file) {
        std::getline(file, current_line); linenum++;

        boost::algorithm::trim(current_line);
        if (current_line == "") continue;

        parse_line(current_line, linenum, current_block, parsed_time_blk, parsed_meta_blk);
    } file.close();

    integrity_check();
}

Block::Block(const Block& other) {
    *this = other;
}

Block::Block() {
    for (size_t i = 0; i < 7; i++) modified[i] = false;
    parse_format = hour_format = title = error_str = link = "";
    link_type = LINK_NA;
    id = group = color = duration = 0;
    collapsible = important = false;
    t_start = {0};
    source_file = "/";
}

void Block::parse_line(std::string line,
                       int line_num,
                       en_parsing_block& cur_block,
                       bool& parsed_time,
                       bool& parsed_meta) {
    std::string error = error_str + " @ line " + std::to_string(line_num)
                        + ", error parsing";

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
        strptime(contents.c_str(), parse_format.c_str(), &t_start);
        t_start.tm_isdst = -1;
        std::mktime(&t_start);
    } else if (name == "group") {
        try {
            group = std::stoi(contents);
        } catch (const std::exception& e) {
            throw std::runtime_error
                (error + ", can't parse group id into integer: " + contents);
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
    } else if (name == "duration") {
        size_t colon_pos = contents.find(":");
        if (colon_pos == std::string::npos)
            throw std::runtime_error
                (error + ", duration doesn't contain a colon: " + contents);

        int hours;
        try {
            hours = std::stoi(contents.substr(0, colon_pos));
        } catch (const std::exception& e) { throw std::runtime_error
            (error + "couldn't parse hour from duration: " + contents);
        } 

        int minutes;
        try {
            minutes = std::stoi(contents.substr(colon_pos + 1));
        } catch (const std::exception& e) { throw std::runtime_error
            (error + "couldn't parse hour from duration: " + contents);
        } 

        duration = minutes*60 + hours*3600;
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
    error_str = "block in file " + source_file.string();
    title = "";
    link = "";
    link_type = LINK_NA;
    color = 0;
    id = 0;
    group = 0;
    collapsible = false;
    important = false;
    t_start = {0};
    duration = 0;
    std::fill(modified, modified + field_count, false);
}

void Block::dump_info() const {
    std::cout << " - Block::dump_info()" << std::endl;
    std::cout << "file: " << source_file.string() << std::endl;
    std::cout << "title: " << title << std::endl;
    std::cout << "id: " << id << std::endl;
    std::cout << "link: " << link << std::endl;
    std::cout << "link_type: " << link_type << std::endl;
    std::cout << "color: " << color << std::endl;
    std::cout << "group: " << group << std::endl;
    std::cout << "collapsible: " << collapsible << std::endl;
    std::cout << "important: " << important << std::endl;
    std::cout << "start: " << get_t_start_str() << std::endl;
    std::cout << "start hr: " << get_t_start_hour_str() << std::endl;
    std::cout << "duration: " << get_duration_str() << std::endl;
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
                           modified[FLD_DURATION];

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

    if (modified[FLD_START] || modified[FLD_DURATION]) {
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
                    if (modified[FLD_DURATION])
                        file_vec[i] += "duration = " + get_duration_str() + "\n";

                    file_vec[i] += "@end";
                    break;
                }

                if ((line.find("start") == 0 && modified[FLD_START]) ||
                    (line.find("duration") == 0 && modified[FLD_DURATION])) {

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
        modified[FLD_DURATION] = false;
    }
}

void Block::integrity_check() const {
    title_integrity(title);
    t_start_integrity(t_start);
    duration_integrity(duration);
    id_integrity(id);
    group_integrity(group);
    color_integrity(color);
    source_file_integrity(source_file);
}

void Block::title_integrity(std::string val) const {
    if (val == "") throw std::runtime_error(error_str+", title is empty");
}

void Block::t_start_integrity(struct tm val) const {
    if (val.tm_year == 0) throw std::runtime_error(error_str+", uninitialized start time");
}

void Block::duration_integrity(time_t val) const {
    if (val%60 != 0) throw std::runtime_error
        (error_str+", duration not in minutes: " + std::to_string(val));

    if (val > 24*60*60) throw std::runtime_error
        (error_str+", duration greater than 24 hours: " + std::to_string(val));

    if (val == 0) throw std::runtime_error
        (error_str+", duration is 0");
}

void Block::id_integrity(int val) const {
    if (val <= 0 || val > 99999) throw std::runtime_error
        (error_str+", id is out of range (0, 99999] ("+std::to_string(val)+")");
}

void Block::group_integrity(int val) const {
    if (val < 0 || val > 99999) throw std::runtime_error
        (error_str+", group is out of range [0, 99999] ("+std::to_string(val)+")");
}

void Block::color_integrity(int val) const {
    if (val < 0 || val > 7) throw std::runtime_error
        (error_str+", color is out of range [0, 7] ("+std::to_string(val)+")");
}

void Block::source_file_integrity(std::filesystem::path val) const {
    if (!std::filesystem::is_regular_file(val)) throw std::runtime_error
        (error_str+", path is not valid");
}

std::string Block::get_duration_str() const {
    int hours = duration / 3600;
    int minutes = (duration % 3600) / 60;

    std::string hours_str = std::to_string(hours);
    std::string minutes_str = ((minutes < 10)? "0" : "") + std::to_string(minutes);

    return hours_str + ":" + minutes_str;
}

std::string Block::get_t_start_str() const {
    char buffer[27];
    struct tm copy = t_start;
    std::strftime(buffer, sizeof(buffer), parse_format.c_str(), &copy);

    return std::string(buffer);
}
std::string Block::get_t_end_hour_str() const {
    char buffer[15];
    time_t end = get_time_t_start() + duration;
    std::strftime(buffer, sizeof(buffer), hour_format.c_str(), localtime(&end));

    return std::string(buffer);
}
std::string Block::get_t_start_hour_str() const {
    char buffer[15];
    struct tm copy = t_start;
    std::strftime(buffer, sizeof(buffer), hour_format.c_str(), &copy);

    return std::string(buffer);
}

std::string Block::get_color_str() const { return color_names[color]; }

std::string Block::get_source_file_str() const { return source_file.string(); }

int Block::get_id() const { return id; }
struct tm Block::get_t_start() const { return t_start; }
std::string Block::get_title() const { return title; }
bool Block::get_collapsible() const { return collapsible; }
bool Block::get_important() const { return important; }
time_t Block::get_duration() const { return duration; }
int Block::get_color() const { return color; }

time_t Block::get_time_t_start() const {
    struct tm copy = t_start;
    return std::mktime(&copy);
}


void Block::set_title(std::string new_title) {
    title = new_title;
    modified[FLD_TITLE] = true;
}

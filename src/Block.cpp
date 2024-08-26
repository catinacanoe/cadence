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
    save_path = cfg_ptr->str({"save_path"});

    while (file) {
        std::getline(file, current_line); linenum++;

        boost::algorithm::trim(current_line);
        if (current_line == "") continue;

        parse_line(current_line, linenum, current_block, parsed_time_blk, parsed_meta_blk);
    } file.close();

    integrity_check();
}

Block::Block(Config* cfg_ptr, int id_) {
    title = cfg_ptr->str({"ui", "boxdrawing", "highlight_fill"});
    error_str = "block initialized from scratch";
    for (size_t i = 0; i < field_count; i++) modified[i] = false;
    link = "";
    link_type = LINK_NA;
    id = id_;
    group = 0;
    color = 0;
    collapsible = false;
    important = false;
    time_t now = time(0); t_start = *std::localtime(&now);
    duration = 60;

    hour_format = cfg_ptr->str({"ui", "date_formats", "hour_format"});
    parse_format = cfg_ptr->str({"ui", "date_formats", "parse_format"});

    modified[FLD_TITLE] = true;
    source_file = "";
    save_path = cfg_ptr->str({"save_path"});
}

Block::Block(const Block& other) {
    *this = other;
}

Block::Block() {
    for (size_t i = 0; i < field_count; i++) modified[i] = false;
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
        } else {
            try {
                std::stoi(contents);
                link_type = LINK_TASK;
            } catch (const std::exception& e) {
                std::string alt_contents = std::regex_replace
                    (contents, std::regex("^~"), getenv("HOME"));

                if (std::filesystem::is_regular_file(contents)) {
                    link_type = LINK_FILE;
                } else if (std::filesystem::is_regular_file(alt_contents)) {
                    link_type = LINK_FILE;
                    link = alt_contents;
                } else {
                    link_type = LINK_HTTP;
                }
            }
        }
    } else if (name == "start") {
        strptime(contents.c_str(), parse_format.c_str(), &t_start);
        t_start.tm_isdst = -1; // set to noop so mktime sets it
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

// public
void Block::set_source_file(std::filesystem::path newfile) {
    source_file = newfile;
    modified[FLD_TITLE] = true;
}

// public
void Block::delete_file() {
    std::remove(source_file.c_str());
    source_file = "";
    set_all_modified();
}

// public
void Block::set_all_modified() {
    bool save = modified[FLD_ID];

    for (size_t i = 0; i < field_count; i++) modified[i] = true;

    modified[FLD_ID] = save;
}

// public
bool Block::follow_link() const {

    switch(link_type) {
        case LINK_NA: return false; break;
        case LINK_FILE: {
            def_prog_mode();
            endwin();

            std::string cmd = "xdg-open \""+link+"\"";
            system(cmd.c_str());

            reset_prog_mode();
            break;
        }
        case LINK_HTTP: {
            std::string cmd = "browse \""+link+"\"";
            system(cmd.c_str());
            break;
        }
        case LINK_TASK:
            // TODO once task manager is implemented
            break;
    }

    return true;
}

// public
void Block::save_to_file() {
    if (modified[FLD_TITLE] || modified[FLD_ID]) {
        if (source_file.string() == "") {
            source_file = save_path + "/TMPFILE.norg";
            std::ofstream tmp_ofstream(source_file);

            tmp_ofstream << "@document.meta" << std::endl;
            tmp_ofstream << "@end" << std::endl;
            tmp_ofstream << std::endl;
            tmp_ofstream << "@code lua time" << std::endl;
            tmp_ofstream << "@end" << std::endl;

            tmp_ofstream.close();
        }

        std::filesystem::path new_file = save_path + "/"
                                       + title + "." + std::to_string(id) + ".norg";

        if (modified[FLD_ID]) { // means a copy was made and so we leave old file intact
            std::filesystem::copy(source_file, new_file);
        } else {
            std::rename(source_file.string().c_str(), new_file.string().c_str());
        }

        source_file = new_file;
        error_str = "block in file " + source_file.string();

        modified[FLD_TITLE] = false;
        modified[FLD_ID] = false;
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
std::filesystem::path Block::get_source_file() const { return source_file; }
std::string Block::get_link() const { return link; }
Block::en_link_type Block::get_link_type() const { return link_type; }

time_t Block::get_time_t_start() const {
    struct tm copy = t_start;
    return std::mktime(&copy);
}

time_t Block::get_date_time() const {
    struct tm copy = t_start;
    copy.tm_hour = copy.tm_min = copy.tm_sec = 0;
    return std::mktime(&copy);
}


time_t Block::get_time_t_end() const { return get_time_t_start() + duration; }

void Block::toggle_important() { set_important(!important); }
void Block::toggle_collapsible() { set_collapsible(!collapsible); }

void Block::set_title(std::string new_title) {
    title = new_title;
    modified[FLD_TITLE] = true;
    title_integrity(title);
}

void Block::set_id(int new_id) {
    id = new_id;
    modified[FLD_ID] = true;
    id_integrity(id);
}

void Block::set_time_t_start(time_t new_start) {
    t_start = *std::localtime(&new_start);
    modified[FLD_START] = true;
    t_start_integrity(t_start);
}

void Block::set_duration(time_t new_duration) {
    duration = new_duration;
    modified[FLD_DURATION] = true;
    duration_integrity(duration);
}

void Block::set_color_str(std::string col) {
    int new_col = -1;

    for (int i = 0; i < 8; i++) {
        if (color_names[i] == col) {
            new_col = i;
            break;
        }
    }

    if (new_col == -1) return;

    color = new_col;
    modified[FLD_COLOR] = true;
    color_integrity(color);
}

void Block::set_important(bool imp) {
    important = imp;
    modified[FLD_IMPORTANT] = true;
}

void Block::set_collapsible(bool coll) {
    collapsible = coll;
    modified[FLD_COLLAPSIBLE] = true;
}

bool operator==(const Block& l, const Block& r) {
    return l.get_time_t_start() == r.get_time_t_start()
    && l.get_time_t_end()   == r.get_time_t_end()
    && l.get_id()           == r.get_id()
    && l.get_title()        == r.get_title()
    && l.get_collapsible()  == r.get_collapsible()
    && l.get_important()    == r.get_important()
    && l.get_color()        == r.get_color()
    && l.get_source_file()  == r.get_source_file();
}

bool operator!=(const Block& l, const Block& r) { return !(l == r); }

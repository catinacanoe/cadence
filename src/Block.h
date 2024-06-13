#pragma once

#include <vector>
#include <filesystem>
#include <boost/algorithm/string/trim.hpp>
#include <ctime>
#include <fstream>
#include <iostream>

class Block {
private:
    std::string title; // the title of the task (brief blurb in the ui)
    std::string error_str; // the intro to all errors
    
    const int field_count = 7; // the number of fields
    bool modified[7]; // keeping track of which fields have been modified
    enum en_fields { FLD_TITLE, FLD_LINK, FLD_COLOR, FLD_COLLAPSIBLE, FLD_IMPORTANT,
                     FLD_START, FLD_DURATION };
    
    enum en_link_type { LINK_NA, LINK_FILE, LINK_HTTP, LINK_TASK };
    std::string link; // can be "", a file path, http link, or an id of a task
    en_link_type link_type; // the type of link we have from the above options

    int id; // the unique id of the task
    int group; // if the task was created as a recurring one
               // it should have a groupid matching all of the other occurrences
               // otherwise it should have a groupid of zero

    int color; // 0-7 corresponding to the color the task should have in ui
    const std::string color_names[8] = { "white", "red", "green", "yellow",
                                         "blue", "purple", "aqua", "gray" };

    bool collapsible; // whether the task should be collapsed to a small size in ui
                      // (even if it has a long duration)

    bool important; // whether or not the task is important (highlighted in ui)

    struct tm t_start;
    time_t duration;

    const char *date_format = "%H:%M~%d.%m.%Y"; // for parsing / writing save files
    const char *hour_format = "%H:%M"; // for ui

    std::filesystem::path source_file; // the save file containing the fields

    enum en_parsing_block { BLK_META, BLK_TIME, BLK_NA };

    void init_fields(); // populate fields with default values
 
    void parse_filename(std::string filename); // populate the id & title fields
    
    void parse_line(std::string line,
                    int line_num,
                    en_parsing_block& cur_block,
                    bool& parsed_time,
                    bool& parsed_meta); // parse the given line

    void parse_field_line(std::string line,
                          std::string delimiter,
                          std::string error,
                          bool& cur_block_parsed,
                          en_parsing_block& cur_block); // parses a line that contains a field

    void init_field(std::string name,
                    std::string contents,
                    std::string error); // initialize a field
    
    void title_integrity(std::string val) const; // the below functions throw error
    void t_start_integrity(struct tm val) const; // if the given field value is invalid
    void duration_integrity(time_t val) const;
    void id_integrity(int val) const;
    void group_integrity(int val) const;
    void color_integrity(int val) const;
    void source_file_integrity(std::filesystem::path val) const;
public:
    Block(std::filesystem::path savefile);
    Block(const Block& other);
    Block();

    void dump_info() const; // just a debug function

    void save_to_file(); // if the current fields don't match the savefile, update it
    
    std::string get_t_start_str() const; // start time as a formatted date string
    std::string get_t_start_hour_str() const; // hour of day of start time
    std::string get_duration_str() const; // start time as a formatted date string
    std::string get_color_str() const; // color in the string name
    std::string get_source_file_str() const; // the source file string
    
    struct tm get_t_start() const;
    time_t get_time_t_start() const;
    int get_id() const;
    std::string get_title() const;
    bool get_collapsible() const;
    time_t get_duration() const;
    
    void integrity_check() const; // check that all field values make sense
    
    Block& operator=(const Block& other) {
        if (this != &other) {
            title = other.title;
            error_str = other.error_str;
            link = other.link;
            link_type = other.link_type;
            id = other.id;
            group = other.group;
            color = other.color;
            collapsible = other.collapsible;
            important = other.important;
            t_start = other.t_start;
            duration = other.duration;
            source_file = other.source_file;

            for (int i = 0; i < field_count; i++) modified[i] = other.modified[i];
        }
        return *this;
    }
};

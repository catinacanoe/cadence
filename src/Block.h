#pragma once

#include "Config.h"

#include <vector>
#include <filesystem>
#include <ncursesw/ncurses.h>
#include <boost/algorithm/string/trim.hpp>
#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <regex>

class Block {
private:
    std::string title; // the title of the task (brief blurb in the ui)
    std::string error_str; // the intro to all errors
    std::string save_path;
    
    const int field_count = 8; // the number of fields
    bool modified[8]; // keeping track of which fields have been modified
    enum en_fields { FLD_ID, FLD_TITLE, FLD_LINK, FLD_COLOR, FLD_COLLAPSIBLE,
                     FLD_IMPORTANT, FLD_START, FLD_DURATION };
    
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

    std::string parse_format; // for parsing / writing save files
    std::string hour_format; // for ui

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
    Block(std::filesystem::path savefile, Config* cfg_ptr);
    Block(Config* cfg_ptr, int id_); // id is the only necessary field
    Block(const Block& other);
    Block();

    void dump_info() const; // just a debug function

    void save_to_file(); // if the current fields don't match the savefile, update it
    void delete_file();
    void set_all_modified();

    void follow_link() const; // open the link
    
    std::string get_t_start_str() const; // start time as a formatted date string
    std::string get_t_end_hour_str() const; // hour of day of end time
    std::string get_t_start_hour_str() const; // hour of day of start time
    std::string get_duration_str() const; // start time as a formatted date string
    std::string get_color_str() const; // color in the string name
    std::string get_source_file_str() const; // the source file string
    
    struct tm get_t_start() const;
    time_t get_time_t_start() const;
    time_t get_time_t_end() const;
    int get_id() const;
    std::string get_title() const;
    bool get_collapsible() const;
    bool get_important() const;
    time_t get_duration() const;
    time_t get_date_time() const;
    int get_color() const;
    std::filesystem::path get_source_file() const;

    void set_title(std::string new_title);
    void set_id(int new_id);
    void set_time_t_start(time_t new_start);
    void set_duration(time_t new_duration);
    void set_source_file(std::filesystem::path newfile);
    void set_color_str(std::string col);
    void set_important(bool imp);
    void set_collapsible(bool coll);

    void toggle_important();
    void toggle_collapsible();
    
    void integrity_check() const; // check that all field values make sense
    
    friend bool operator==(const Block& l, const Block& r);
    friend bool operator!=(const Block& l, const Block& r);

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
            hour_format = other.hour_format;
            parse_format = other.parse_format;
            save_path = other.save_path;

            for (int i = 0; i < field_count; i++) modified[i] = other.modified[i];
        }
        return *this;
    }
};

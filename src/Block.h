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
    
    const int field_count = 7; // the number of fields
    bool modified[7]; // keeping track of which fields have been modified
    enum en_fields { FLD_TITLE, FLD_LINK, FLD_COLOR, FLD_COLLAPSIBLE, FLD_IMPORTANT, FLD_START, FLD_END };
    
    enum en_link_type { LINK_NA, LINK_FILE, LINK_HTTP, LINK_TASK };
    std::string link; // can be "", a file path, http link, or an id of a task
    en_link_type link_type; // the type of link we have from the above options

    int id; // the unique id of the task
    int group; // if the task was created as a recurring one
               // it should have a groupid matching all of the other occurrences
               // otherwise it should have a groupid of zero

    int color; // 0-7 corresponding to the color the task should have in ui
    const std::string color_names[8] = { "white", "red", "green", "yellow", "blue", "purple", "aqua", "gray" };

    bool collapsible; // whether the task should be collapsed to a small size in ui
                      // (even if it has a long duration)

    bool important; // whether or not the task is important (highlighted in ui)

    struct tm t_start;
    struct tm t_end; // the start and end times of this block
    const char *date_format = "%H:%M~%d.%m.%Y";
    char date_buffer[17];

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
    
public:
    Block(std::filesystem::path savefile);

    void dump_info(); // just a debug function

    void save_to_file(); // if the current fields don't match the savefile, update it

    void integrity_check(); // check that the field values make sense
    
    std::string get_t_start_str(); // start time as a formatted date string
    std::string get_t_end_str(); // start time as a formatted date string
    std::string get_color_str(); // color in the string name
};

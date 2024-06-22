#include "Ui.h"

#include <vector>
#include <string>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for(size_t i = 0; i < argc; i++)
        args.push_back(argv[i]);

    Ui ui(args);
    ui.main();
}

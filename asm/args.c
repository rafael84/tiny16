#include "args.h"
#include "easyargs.h"

args_t args;

void make_and_parse_args(int argc, char** argv) {
    args = make_default_args();
    if (!parse_args(argc, argv, &args) || args.help || *args.source_filename == '-') {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
}

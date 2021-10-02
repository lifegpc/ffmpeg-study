#include "getopt.h"
#include "wchar_util.h"
#include <list>
#include "ffconcat.h"

#if _WIN32
#include "Windows.h"
#endif

#if _WIN32
#define printf printf_s
#endif

void print_help() {
    printf("%s", "Usage: main [options] [FILE [..]]\n\
Concat video files\n\
\n\
Options:\n\
    -h, --help              Print help message.\n\
    -o, --output [FILE]     Specifiy output file location. Default output location: a.mp4.\n\
    -v, --verbose           Enable verbose logging\n");
}

int main(int argc, char* argv[]) {
#if _WIN32
    SetConsoleOutputCP(CP_UTF8);
    bool have_wargv = false;
    int wargc;
    char **wargv;
    if (wchar_util::getArgv(wargv, wargc)) {
        have_wargv = true;
        argc = wargc;
        argv = wargv;
    }
#endif
    struct option opts[] = {
        {"help", 0, nullptr, 'h'},
        {"output", 1, nullptr, 'o'},
        {"verbose", 0, nullptr, 'v'},
        nullptr,
    };
    int c;
	const char* shortopts = "-ho:v";
    std::string output = "a.mp4";
    std::list<std::string> li;
    bool verbose = false;
    while ((c = getopt_long(argc, argv, shortopts, opts, nullptr)) != -1) {
        switch (c) {
            case 'h':
                print_help();
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 0;
            case 'o':
                output = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 1:
                li.push_back(optarg);
                break;
            case '?':
		    default:
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
        }
    }
#if _WIN32
    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
    if (verbose) {
        printf("Output file: %s\n", output.c_str());
        for(auto i = li.begin(); i != li.end(); i++) {
            printf("Input file: %s\n", (*i).c_str());
        }
    }
    ffconcath conf;
    conf.verbose = verbose;
    int re = ffconcat(output, li, conf);
    return re;
}

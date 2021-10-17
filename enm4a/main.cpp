#if HAVE_ENM4A_CONFIG_H
#include "enm4a_config.h"
#endif
#include "enm4a_version.h"

#include "getopt.h"
#include "wchar_util.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "enm4a.h"
#include "cpp2c.h"

#if _WIN32
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

void print_help() {
    printf("%s", "Usage: enm4a [options] FILE\n\
Convert file to m4a file\n\
\n\
Options:\n\
    -h, --help              Print help message.\n\
    -o, --output <FILE>     Specifiy output file location. Default output location: <title>.m4a.\n\
                            If title is not found, will use input filename instead.\n\
    -v, --verbose           Enable verbose logging.\n\
        --debug             Enable debug logging.\n\
        --trace             Enable trace logging.\n\
    -t, --title <title>     Sepcify title of song.\n\
    -c, --cover <path>      Sepcify cover image.\n\
    -a, --artist <name>     Sepcify artist's name.\n\
    -A, --album <name>      Sepcify album's name.\n\
        --album_artist <name>   Specify album artist.\n\
    -d, --disc <num>        Specify disc number.\n\
    -T, --track <track>     Specify track number.\n\
    -D, --date <date>       Specify release date/year.\n\
    -y, --yes               Overwrite file if output file is already existed.\n\
    -n, --no                Don't overwrite file if output file is already existed.\n\
    -V, --version           Print version.\n");
}

void print_version(bool verbose) {
    printf("enm4a v%s Copyright (C) 2021  lifegpc\n\
Source code: https://github.com/lifegpc/ffmpeg-study/tree/master/enm4a \n\
This program comes with ABSOLUTELY NO WARRANTY;\n\
for details see <https://www.gnu.org/licenses/agpl-3.0.html>.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions.\n", ENM4A_VERSION);
    enm4a_print_ffmpeg_version();
    if (verbose) {
        enm4a_print_ffmpeg_configuration();
        enm4a_print_ffmpeg_license();
    } else {
        printf("Add \"-v\" to see more inforamtion about ffmpeg library.\n");
    }
}

#define ENM4A_TRACE 129
#define ENM4A_ALBUM_ARTIST 130
#define ENM4A_DEBUG 131

int main(int argc, char* argv[]) {
#if _WIN32
    SetConsoleOutputCP(CP_UTF8);
    bool have_wargv = false;
    int wargc;
    char** wargv;
    if (wchar_util::getArgv(wargv, wargc)) {
        have_wargv = true;
        argc = wargc;
        argv = wargv;
    }
#endif
    if (argc == 1) {
        print_help();
#if _WIN32
        if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
        return 0;
    }
    struct option opts[] = {
        {"help", 0, nullptr, 'h'},
        {"output", 1, nullptr, 'o'},
        {"verbose", 0, nullptr, 'v'},
        {"debug", 0, nullptr, ENM4A_DEBUG},
        {"trace", 0, nullptr, ENM4A_TRACE},
        {"title", 1, nullptr, 't'},
        {"cover", 1, nullptr, 'c'},
        {"artist", 1, nullptr, 'a'},
        {"album", 1, nullptr, 'A'},
        {"album_artist", 1, nullptr, ENM4A_ALBUM_ARTIST},
        {"album-artist", 1, nullptr, ENM4A_ALBUM_ARTIST},
        {"disc", 1, nullptr, 'd'},
        {"track", 1, nullptr, 'T'},
        {"date", 1, nullptr, 'D'},
        {"yes", 0, nullptr, 'y'},
        {"no", 0, nullptr, 'n'},
        {"version", 0, nullptr, 'V'},
        nullptr,
    };
    int c;
    const char* shortopts = "-ho:vd:t:c:a:A:T:D:ynV";
    std::string output = "";
    std::string input = "";
    ENM4A_LOG level = ENM4A_LOG_INFO;
    std::string title = "";
    std::string cover = "";
    std::string artist = "";
    std::string album = "";
    std::string album_artist = "";
    std::string disc = "";
    std::string track = "";
    std::string date = "";
    ENM4A_OVERWRITE overwrite = ENM4A_OVERWRITE_ASK;
    bool printv = false;
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
            level = ENM4A_LOG_VERBOSE;
            break;
        case ENM4A_DEBUG:
            level = ENM4A_LOG_DEBUG;
            break;
        case ENM4A_TRACE:
            level = ENM4A_LOG_TRACE;
            break;
        case 't':
            title = optarg;
            break;
        case 'c':
            cover = optarg;
            break;
        case 'a':
            artist = optarg;
            break;
        case 'A':
            album = optarg;
            break;
        case ENM4A_ALBUM_ARTIST:
            album_artist = optarg;
            break;
        case 'd':
            disc = optarg;
            break;
        case 'T':
            track = optarg;
            break;
        case 'D':
            date = optarg;
            break;
        case 'y':
            overwrite = ENM4A_OVERWRITE_YES;
            break;
        case 'n':
            overwrite = ENM4A_OVERWRITE_NO;
            break;
        case 'V':
            printv = true;
            break;
        case 1:
            if (!input.length()) {
                input = optarg;
            } else {
                printf("%s\n", "Too much input files.");
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
            }
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
    if (printv) {
        print_version(level >= ENM4A_LOG_VERBOSE);
        return 0;
    }
    if (!input.length()) {
        printf("%s\n", "An input file is needed.");
        return 1;
    }
    if (level >= ENM4A_LOG_VERBOSE) {
        printf("Get input file name: %s\n", input.c_str());
    }
    ENM4A_ARGS arg;
    memset(&arg, 0, sizeof(ENM4A_ARGS));
    arg.level = level;
    arg.overwrite = overwrite;
    if (output.length()) {
        if (!cpp2c::string2char(output, arg.output)) return 1;
    }
    if (title.length()) {
        if (!cpp2c::string2char(title, arg.title)) return 1;
    }
    if (cover.length() && !cpp2c::string2char(cover, arg.cover)) return 1;
    if (artist.length() && !cpp2c::string2char(artist, arg.artist)) return 1;
    if (album.length() && !cpp2c::string2char(album, arg.album)) return 1;
    if (album_artist.length() && !cpp2c::string2char(album_artist, arg.album_artist)) return 1;
    if (disc.length() && !cpp2c::string2char(disc, arg.disc)) return 1;
    if (track.length() && !cpp2c::string2char(track, arg.track)) return 1;
    if (date.length() && !cpp2c::string2char(date, arg.date)) return 1;
    ENM4A_ERROR re = encode_m4a(input.c_str(), arg);
    if (arg.output) {
        free(arg.output);
    }
    if (arg.title) {
        free(arg.title);
    }
    if (arg.cover) free(arg.cover);
    if (arg.artist) free(arg.artist);
    if (arg.album) free(arg.album);
    if (arg.album_artist) free(arg.album_artist);
    if (arg.disc) free(arg.disc);
    if (arg.track) free(arg.track);
    if (arg.date) free(arg.date);
    if (re != ENM4A_OK && re != ENM4A_FILE_EXISTS) {
        if (re != ENM4A_FFMPEG_ERR) {
            printf("%s\n", enm4a_error_msg(re));
        }
        return 1;
    }
    return 0;
}

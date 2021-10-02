#ifndef _FFCONCAT_FFCONCAT_H
#define _FFCONCAT_FFCONCAT_H
#include <string>
#include <list>

typedef struct ffconcath {
    bool verbose = false;
} ffconcath;

int ffconcat(std::string out, std::list<std::string> inp, ffconcath config);

#endif

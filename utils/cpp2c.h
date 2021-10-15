#ifndef _UTILS_CPP2C_H
#define _UTILS_CPP2C_H

#include <string>

namespace cpp2c {
    /**
     * @brief Convert string to char*
     * @param inp Input string.
     * @param out The pointer to output buffer. Need free memory by using free.
     * @return true if succeeded.
    */
    bool string2char(std::string inp, char** out);
}

#endif

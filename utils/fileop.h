#ifndef _UTIL_FILEOP_H
#define _UTIL_FILEOP_H
#include <string>

namespace fileop {
    /**
     * @brief Check file exists
     * @param fn File name
     * @returns Whether file exists or not
    */
    bool exists(std::string fn);
    /**
     * @brief Remove file
     * @param fn File name
     * @param print_error Print error message
     * @returns true if successed
    */
    bool remove(std::string fn, bool print_error = false);
}
#endif

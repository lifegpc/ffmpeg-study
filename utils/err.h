#ifndef _UTILS_ERR_H
#define _UTILS_ERR_H
#include <string>

namespace err {
    /**
     * @brief Get error message from errno
     * @param out Output string
     * @param errnum errno
     * @returns true if successed
    */
    bool get_errno_message(std::string &out, int errnum);
}
#endif

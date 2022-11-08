//
// Created by yuanlu on 2022/10/18.
//
#include "config.h"

namespace ifr {
    namespace Config {
        void mkDir(std::string path) {
            if (path.empty())return;
#if __OS__ == __OS_Windows__
            std::replace(path.begin(), path.end(), '/', '\\');
            system(("mkDir \"" + path + "\" >nul 2>nul").c_str());
#elif __OS__ == __OS_Linux__
            std::replace(path.begin(), path.end(), '\\', '/');
            system(("mkdir -p \"" + path + "\" 1>/dev/null 2>/dev/null").c_str());
#endif
        }

        std::string getDir(std::string fname) {
#if __OS__ == __OS_Windows__
            static const char _f = '/', _t = '\\';
#elif __OS__ == __OS_Linux__
            static const char _f = '\\', _t = '/';
#endif
            std::replace(fname.begin(), fname.end(), _f, _t);
            size_t index;
            if ((index = fname.find_last_of(_t)) == std::string::npos)return "";
            return fname.substr(0, index);
        }
    };

} // ifr

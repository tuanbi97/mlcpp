#include "fileutil.h"
#include <iostream>
#include <sstream>

int IsBigEndian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

void GetAllFileWithExt(const fs::path &root,
                       const std::string &ext,
                       std::vector<fs::path> &ret)
{
    if (!fs::exists(root) || !fs::is_directory(root))
    {
        std::cout << "Path: " << root << " doesn't exist";
        return;
    }

    fs::recursive_directory_iterator it(root);
    fs::recursive_directory_iterator endit;

    while (it != endit)
    {
        if (fs::is_regular_file(*it) && it->path().extension() == ext)
        {
            ret.push_back(it->path().filename());
        }
        ++it;
    }
}

std::vector<std::string> SplitString(const std::string &s, char delim)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> tokens;
    while (getline(ss, item, delim))
    {
        tokens.push_back(item);
    }
    return tokens;
}

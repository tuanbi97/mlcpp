#include <string>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

/// \brief Whether current system is big endian
int IsBigEndian(void);

/// \brief Split the strings from delimiter
/// \param s[in]      : input string
/// \param delim[in]  : delimiter
/// \ret The split strings in vector
std::vector<std::string> SplitString(const std::string &s,
                                     char delim);

/// \brief Find all the files with extension in a folder
/// \param path[in] : input path
/// \param ext[in]  : extension
/// \param ret[out] : found paths
void GetAllFileWithExt(const fs::path &path,
                       const std::string &ext,
                       std::vector<fs::path> &ret);

#if defined(_WIN32) || defined(_WIN64)
#define AYR_WIN
#include "fs/win/File.hpp"
#include "fs/win/Path.hpp"

#elif defined(__linux__) || defined(__unix__)
#define AYR_LINUX
#include "fs/linux/File.hpp"
#include "fs/linux/Path.hpp"

#elif defined(__APPLE__)
#define AYR_MAC
#include "fs/mac/File.hpp"
#include "fs/mac/Path.hpp"
#endif




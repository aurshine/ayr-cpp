#if defined(_WIN32) || defined(_WIN64)
#define AYR_WIN
#include "win/File.hpp"
#include "win/Path.hpp"

#elif defined(__linux__) || defined(__unix__)
#define AYR_LINUX
#include "linux/File.hpp"
#include "linux/Path.hpp"

#elif defined(__APPLE__)
#define AYR_MAC
#include "mac/File.hpp"
#include "mac/Path.hpp"
#endif




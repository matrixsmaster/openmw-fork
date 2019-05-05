#ifndef APPS_OPENMW_MAIN_HPP_
#define APPS_OPENMW_MAIN_HPP_

#include <components/version/version.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/escape.hpp>
#include <components/fallback/validate.hpp>
#include <components/debug/debugging.hpp>
#include <components/misc/rng.hpp>

#include "engine.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// makes __argc and __argv available on windows
#include <cstdlib>
#endif

#if (defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix))
#include <unistd.h>
#endif

/**
 * Workaround for problems with whitespaces in paths in older versions of Boost library
 */
#if (BOOST_VERSION <= 104600)
namespace boost
{

template<>
inline boost::filesystem::path lexical_cast<boost::filesystem::path, std::string>(const std::string& arg)
{
    return boost::filesystem::path(arg);
}

} /* namespace boost */
#endif /* (BOOST_VERSION <= 104600) */

// Platform specific for Windows when there is no console built into the executable.
// Windows will call the WinMain function instead of main in this case, the normal
// main function is then called with the __argc and __argv parameters.
#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return main(__argc, __argv);
}
#endif

#endif /* APPS_OPENMW_MAIN_HPP_ */

#include "version.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

namespace Version
{

Version getOpenmwVersion(const std::string &resourcePath)
{
#if 0
    boost::filesystem::path path (resourcePath + "/version");

    boost::filesystem::ifstream stream (path);

    Version v;
    std::getline(stream, v.mVersion);
    std::getline(stream, v.mCommitHash);
    std::getline(stream, v.mTagHash);
    return v;
#else
    return Version();
#endif
}

std::string Version::describe()
{
    std::string str = "OpenMW version " + mVersion;

    if (!mCommitHash.empty())
        str += "\nPrev. MSM Branch hash: " + mCommitHash.substr(0, 10);

    return str;
}

std::string getOpenmwVersionDescription(const std::string &resourcePath)
{
    Version v = getOpenmwVersion(resourcePath);
    return v.describe();
}

}

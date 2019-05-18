#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>

namespace Version
{

    struct Version
    {
    	// I have to hard-code it because the version resource file in my setup is too volatile
    	// and could completely be out of sync with the current ELF file being executed
        const std::string mVersion = "0.45.0";
        const std::string mCommitHash = "3b041502aa5ebd424943b2336cc20b963c4c3713"; // this would point to the PREVIOUS commit

        std::string describe();
    };

    /// Read OpenMW version from the version file located in resourcePath.
    Version getOpenmwVersion(const std::string& resourcePath);

    /// Helper function to getOpenmwVersion and describe() it
    std::string getOpenmwVersionDescription(const std::string& resourcePath);

}


#endif // VERSION_HPP


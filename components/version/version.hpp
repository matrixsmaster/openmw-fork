#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>

namespace Version
{

    struct Version
    {
    	// I have to hard-code it because the version resource file in my setup is too volatile
    	// and could completely be out of sync with the current ELF file being executed
        const std::string mVersion = "0.45.5 DOS";
        const std::string mCommitHash = "ce7fdfdd6b0b2da5ec41f1d654e083f70f167daf"; // this would point to the PREVIOUS commit

        std::string describe();
    };

    /// Read OpenMW version from the version file located in resourcePath.
    Version getOpenmwVersion(const std::string& resourcePath);

    /// Helper function to getOpenmwVersion and describe() it
    std::string getOpenmwVersionDescription(const std::string& resourcePath);

}


#endif // VERSION_HPP


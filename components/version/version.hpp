#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>

namespace Version
{

    struct Version
    {
    	// I have to hard-code it because the version resource file in my setup is too volatile
    	// and could completely be out of sync with the current ELF file being executed
        const std::string mVersion = "0.45.2 DOS";
        const std::string mCommitHash = "b3fbe5425f279830c30e770c8eeb11c99ee85abb"; // this would point to the PREVIOUS commit

        std::string describe();
    };

    /// Read OpenMW version from the version file located in resourcePath.
    Version getOpenmwVersion(const std::string& resourcePath);

    /// Helper function to getOpenmwVersion and describe() it
    std::string getOpenmwVersionDescription(const std::string& resourcePath);

}


#endif // VERSION_HPP


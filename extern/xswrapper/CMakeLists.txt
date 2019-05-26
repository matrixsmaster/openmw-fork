set(XS_DOS_WRAPPER_LIBRARY "xswrapper")

# Sources

set(XS_DOS_WRAPPER_SOURCE_FILES
    xswrapper.cpp
    xswrapper.hpp
    soundr.hpp
    xskbd.hpp
)

add_library(${XS_DOS_WRAPPER_LIBRARY} STATIC ${XS_DOS_WRAPPER_SOURCE_FILES})

target_link_libraries(${XS_DOS_WRAPPER_LIBRARY} ldosbox)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

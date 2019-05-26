set(XS_DOS_WRAPPER_LIBRARY "xswrapper")

set(XS_DOSCARD_PATH "/mnt/data/dev/doscard/PC-Heavy")

# Sources

set(XS_DOS_WRAPPER_SOURCE_FILES
    xswrapper.cpp
    xswrapper.hpp
    soundr.hpp
    xskbd.hpp
)

include_directories("${XS_DOSCARD_PATH}/box/include")
link_directories("${XS_DOSCARD_PATH}/box/src")

add_library(${XS_DOS_WRAPPER_LIBRARY} STATIC ${XS_DOS_WRAPPER_SOURCE_FILES})

add_library(ldosbox STATIC IMPORTED)
set_target_properties(ldosbox PROPERTIES IMPORTED_LOCATION "${XS_DOSCARD_PATH}/box/src/libdosbox.a")

target_link_libraries(${XS_DOS_WRAPPER_LIBRARY} ldosbox)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

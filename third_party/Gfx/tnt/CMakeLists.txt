cmake_minimum_required(VERSION 3.19)
project(Gfx)

add_library(Gfx INTERFACE)
target_include_directories(Gfx INTERFACE ${GFX_INCLUDE_DIR})

# Uncomment the following lines and specify your folder containing Gfx libraries
# to enable linking sample projects using Gfx. You have to do this with the root
# CMakeLists.txt of the `samples` folder too.
# set(GFX_LIBS_DIR "")
# set(GFX_LIBS "")
# target_link_directories(Gfx INTERFACE ${GFX_LIBS_DIR})
# target_link_libraries(Gfx INTERFACE ${GFX_LIBS})
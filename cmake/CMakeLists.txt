cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD, 14)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} )


add_executable(GBEmu 
	../../GBEmulator/GBEmulator.cpp 
	../../GBEmulator/Cartridge.cpp 
	../../GBEmulator/Gameboy.cpp)

target_link_libraries(GBEmu ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} )

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

cmake_minimum_required(VERSION 3.10)
project(AxeBound LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# SFML 2.5 (graphics, window, system, audio). box2d from the original file is not used by the game so it was dropped.
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

add_executable(AxeBound
  src/main.cpp src/render.cpp src/update.cpp src/init.cpp src/inputManager.cpp src/globals.cpp
  src/objects.cpp src/tileMap.cpp src/levels.cpp src/saveManager.cpp src/audio.cpp
  src/menuScene.cpp src/gameScene.cpp src/gameUi.cpp src/gameSystem.cpp)
target_include_directories(AxeBound PRIVATE include)
target_link_libraries(AxeBound PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

# copy assets/ next to the executable so it also runs from build/
add_custom_command(TARGET AxeBound POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:AxeBound>/assets)

cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(eshywm VERSION 1.0)

option(BUILD_SHARED_LIBS ON)

find_package(X11 REQUIRED)

set(BIN_NAME eshywm)
set(SOURCE_FILES main.cpp background.cpp image.cpp system.cpp util.cpp X11.cpp config.cpp eshywm.cpp window.cpp container.cpp window_manager.cpp menu_base.cpp switcher.cpp button.cpp)
list(TRANSFORM SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

add_executable(${BIN_NAME} ${SOURCE_FILES})
target_include_directories(${BIN_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source/includes)
target_link_libraries(${BIN_NAME} PUBLIC X11 /usr/lib/libXrandr.so /usr/lib/libImlib2.so)
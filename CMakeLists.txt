cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(eshywm VERSION 1.0)

find_package(X11 REQUIRED)
find_package(glog REQUIRED)

set(BIN_NAME eshywm)
set(SOURCE_FILES main.cpp eshywm.cpp window.cpp window_manager.cpp container.cpp taskbar.cpp button.cpp)
list(TRANSFORM SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)

add_executable(${BIN_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp ${SOURCE_FILES})
target_include_directories(${BIN_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source/includes)
target_link_libraries(${BIN_NAME} PUBLIC X11 glog)
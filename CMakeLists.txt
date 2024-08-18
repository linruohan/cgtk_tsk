cmake_minimum_required(VERSION 3.29)
project(cgtk_tsk C)

set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(ADWAITA libadwaita-1 REQUIRED)

include_directories(${GTK4_INCLUDE_DIRS})
link_directories(${GTK4_LIBRARY_DIRS})



add_executable(cgtk_tsk main.c)
target_link_libraries(cgtk_tsk ${GTK4_LIBRARIES} ${ADWAITA_LIBRARIES})
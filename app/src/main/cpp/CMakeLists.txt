
cmake_minimum_required(VERSION 3.4.1)

include_directories(include)

file(GLOB my_source_path ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.c)

add_library(function_control SHARED ${my_source_path})

add_library(lib_opencv SHARED IMPORTED)
add_library(lib_share SHARED IMPORTED)

set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../libs/${ANDROID_ABI}/libopencv_java4.so)
set_target_properties(lib_share PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../libs/${ANDROID_ABI}/libc++_shared.so)

find_library(log-lib log)

target_link_libraries(function_control jnigraphics ${log-lib} lib_opencv lib_share)
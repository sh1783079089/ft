cmake_minimum_required(VERSION 3.20)
project(file_tracker C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(file_tracker
        src/main.c
        src/tracker.c
        src/storage.c
        src/lib.c
)

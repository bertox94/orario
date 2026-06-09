cmake_minimum_required(VERSION 4.2)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c
        professors.h
        days.h
        hours.h
        rooms.h
        classes.h
        subjects.h
)

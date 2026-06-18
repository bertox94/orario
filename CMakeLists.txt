cmake_minimum_required(VERSION 4.2)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c
        constants/professors.h
        constants/days.h
        constants/hours.h
        constants/rooms.h
        constants/classes.h
        constants/subjects.h
        data_initialization/database_initializers.h
        data_initialization/database_initializers.c
)

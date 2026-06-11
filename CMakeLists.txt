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
        database_initializers.h
        find_positions.h
        find_positions.c
        printing.h
        printing.c
        constraints/progressive_constraints.h
        constraints/end_of_block_constraints.h
        database_initializers.c
        constraints/progressive_constraints.c
        professor_functions.c
        professor_functions.h
)

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
        database_initializers.h
        find_positions.h
        find_positions.c
        printing.h
        printing.c
        progressive_constraints.h
        end_of_block_constraints.h
        database_initializers.c
        progressive_constraints.c
        professor_functions.c
        professor_functions.h
)

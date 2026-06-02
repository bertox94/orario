cmake_minimum_required(VERSION 4.2)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c
        orario.h
        ora_giorno_classe.h
        orario.c)

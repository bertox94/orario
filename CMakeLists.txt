cmake_minimum_required(VERSION 4.2)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c
        constants/professors.h
        constants/days.h
        constants/hours.h
        constants/classes.h
        data_initialization/database_initializers.h
        data_initialization/database_initializers.c
        data_initialization/dati_di_prova.c
        data_initialization/nomi_docenti.h
        data_initialization/generatore.h
        data_initialization/generatore.c
        ricerca.h
        ricerca.c
        caso.h
        caso.c
        passi.h
        passi.c
        prove.h
        prove.c
        priority_queue/prio_queue.c
        priority_queue/prio_queue.h
        utils.h
)

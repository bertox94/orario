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
        data_initialization/generatore.h
        data_initialization/generatore.c
        orario/orario.h
        orario/orario.c
        ricerca/ricerca.h
        ricerca/ricerca.c
        ricerca/vincoli.h
        ricerca/vincoli.c
        ricerca/diagnosi.h
        ricerca/diagnosi.c
        ricerca/fattibilita.h
        ricerca/fattibilita.c
        printer/printer.h
        printer/printer.c
        printer/risultati_json.h
        printer/risultati_json.c
        priority_queue/prio_queue.c
        priority_queue/prio_queue.h
        utils.h
)

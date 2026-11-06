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
        printer/printer.h
        printer/printer.c
        priority_queue/prio_queue.c
        priority_queue/prio_queue.h
        utils.h
        ingresso_uscita/json.h
        ingresso_uscita/json.c
        ingresso_uscita/dati_json.h
        ingresso_uscita/dati_json.c
        ingresso_uscita/traccia.h
        ingresso_uscita/traccia.c
        potature/potature.h
        potature/potature.c
)

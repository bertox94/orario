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
        verifica_dati.h
        verifica_dati.c
        verifica_orario.h
        verifica_orario.c
        ricerca.h
        ricerca.c
        caso.h
        caso.c
        passi.h
        passi.c
        vincoli.h
        ultima.h
        ultima.c
        passo.h
        passo.c
        json.h
        prove.h
        prove.c
        priority_queue/prio_queue.c
        priority_queue/prio_queue.h
)

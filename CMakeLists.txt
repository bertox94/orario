cmake_minimum_required(VERSION 4.2)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled main.c
        dati/database_initializers.h
        dati/database_initializers.c
        dati/nomi_docenti.h
        dati/generatore.h
        dati/generatore.c
        verifiche/verifica_dati.h
        verifiche/verifica_dati.c
        verifiche/verifica_orario.h
        verifiche/verifica_orario.c
        ricerca/ricerca.h
        ricerca/ricerca.c
        ricerca/vincoli.h
        ricerca/potature.h
        ricerca/potature.c
        ricerca/coda/prio_queue.c
        ricerca/coda/prio_queue.h
        comandi/caso.h
        comandi/caso.c
        comandi/passi.h
        comandi/passi.c
        comandi/passo.h
        comandi/passo.c
        comandi/json.h
        comandi/prove.h
        comandi/prove.c
)

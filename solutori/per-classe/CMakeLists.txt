cmake_minimum_required(VERSION 3.20)
project(solutore_per_classe C)
set(CMAKE_C_STANDARD 11)
# i sorgenti condivisi sono quelli della radice del repository (../..); qui solo la ricerca
add_executable(solutore_per_classe main.c
        ../../data_initialization/database_initializers.c
        ../../printer/printer.c
        ../../priority_queue/prio_queue.c
        ../../ingresso_uscita/json.c
        ../../ingresso_uscita/dati_json.c
        ../../potature/potature.c
        ../../controllo_in_avanti/controllo_in_avanti.c
        ../../ingresso_uscita/traccia.c
)

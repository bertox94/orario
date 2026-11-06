cmake_minimum_required(VERSION 3.20)
project(solutore_in_avanti C)
set(CMAKE_C_STANDARD 11)
# i sorgenti condivisi sono quelli della radice del repository (../..); qui la ricerca e il suo controllo nella traccia
add_executable(solutore_in_avanti main.c traccia_in_avanti.c
        ../../data_initialization/database_initializers.c
        ../../printer/printer.c
        ../../priority_queue/prio_queue.c
        ../../ingresso_uscita/json.c
        ../../ingresso_uscita/dati_json.c
        ../../potature/potature.c
        ../../ingresso_uscita/traccia.c
        ../../ingresso_uscita/prova.c
)

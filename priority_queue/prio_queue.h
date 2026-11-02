//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRIO_QUEUE_H
#define UNTITLED_PRIO_QUEUE_H

// La coda dei docenti: in ogni slot la ricerca prova i docenti in ordine di priorita' decrescente.
// La priorita' di un docente sono le ore che gli restano da fare in tutta la settimana: parte dalle sue ore settimanali
// e la ricerca la abbassa di 1 a ogni ora assegnata (e la rialza quando la annulla). Cosi' si prova prima chi ha piu'
// ore ancora da sistemare.

// Un docente nella coda. Ce n'e' uno solo per docente, condiviso da tutti gli slot.
typedef struct el {
    int teacher;
    double priority;
} QueueItem, *elp;

// Una copia della coda (vettore di puntatori agli stessi QueueItem) per ogni slot possibile: quella dello slot inizia
// a queue_mul[slot->base4d_queue] ed e' lunga queue_sz. Ogni slot ordina la sua copia senza disturbare quella degli
// slot meno profondi, che la ricerca sta ancora scorrendo.
extern QueueItem **queue_mul;
extern int queue_sz; // quanti docenti ci sono nella coda

// L'ordine della coda subito dopo il caricamento dei dati (la ricerca poi riordina le copie, compresa quella del primo
// slot, che e' anche la coda originale). Lo scrive il JSON, per poter rifare la stessa ricerca (vedi dati_da_file.h).
extern int ordine_iniziale_coda[];

void ricorda_ordine_iniziale_coda(void);

// Aggiunge `priority` ore al docente prof (inserendolo in tutte le copie della coda se non c'e' ancora).
void add_to_queue(int prof, int priority);

#endif //UNTITLED_PRIO_QUEUE_H

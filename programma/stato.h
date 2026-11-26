#ifndef STATO_H
#define STATO_H

#include <stdio.h>

#include "../solver/ricerca.h"

// Lo stato di una ricerca, in JSON: e' quello che il sito legge per sapere a che punto e' e com'e' finita.
//
//   {"in_corso": 0 | 1, "esito": "in corso" | "trovato" | "impossibile" | "limite di nodi",
//    "nodi": N, "secondi": S, "profondita_massima": P, "celle": C,
//    "a_mano": 0 | 1                            1: la ricerca e' partita da una strada scelta a mano (--da)
//    "ritorni": R, "ritorni_per_cella": [...]   i ritorni indietro, in tutto e per cella (ricerca.h)
//    "ritorni_recenti_per_cella": [...]         per cella, solo quelli dall'ultimo passo avanti (ricerca.h)
//    "ritorni_per_motivo": [[tipo, docente, classe, quanti, recenti], ...]   per controllo e per chi il motivo
//                                               nomina (un docente, o -1 e una classe): solo quelli che ne hanno
//    "scuola": {"giorni": [...], "ore": [...], "classi": [...], "obbligatorie": M, "ore_di_seguito": N, "classi_al_giorno": N,
//               "docenti": [[nome, ingresso], ...], "voci": [[classe, docente, codocente | -1, ore], ...]},
//    "motivo": {...}              con «impossibile» gia' in partenza: il conto che non torna (ricerca.h, Motivo)
//    "orario": [voce | -1, ...]   con «trovato»: una voce per cella (-1: ora buca); la cella di (giorno, ora, classe)
//                                 e' (giorno * ore + ora) * classi + classe
//    "verifica": {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}   con «trovato» (verifica.h)}

// Due pezzi dello stato, che servono anche da soli (i comandi «scuola» e «verifica» di main.c): la scuola, come
// "scuola": {...}, e un orario completo con la sua verifica, come "orario": [...], "verifica": {...}.
void scrivi_la_scuola(FILE *f, const Scuola *s);
void scrivi_orario_e_verifica(FILE *f, const Scuola *s, const int *orario);

// Scrive lo stato su `f`. `secondi`: da quanto la ricerca va; `a_mano`: 1 se e' partita da una strada scelta a mano.
void scrivi_stato(FILE *f, const Ricerca *r, double secondi, int a_mano);

#endif

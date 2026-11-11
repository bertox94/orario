#ifndef UNTITLED_ULTIMA_H
#define UNTITLED_ULTIMA_H

// La cella piu' avanti, per il sito: ./untitled ultima SEME CLASSI RITARDI PASSI [VISITE]
//
// La cella piu' avanti e' la piu' lontana a cui la ricerca e' arrivata: li' non e' mai riuscita a mettere niente,
// quindi ogni volta che ci arriva (una visita) torna indietro. Rilegge il file PASSI (passi.h) dall'inizio alla fine
// rifacendo lo stato della ricerca nodo per nodo, e a ogni visita di quella cella valuta i controlli dei docenti della
// classe con le funzioni della ricerca (vincoli.h). Stampa su stdout, in JSON:
// - "pos": la cella, "celle": quante celle ha l'orario, "visite": quante volte la ricerca ci e' arrivata;
// - "docenti": per ogni docente della classe, in quante visite ogni controllo era vero ("controlli": [[nome, n], ...]),
//   in quante poteva insegnare ("puo"), e lo stesso da solo ("da_solo");
// - "coppie": per ogni coppia con ore insieme nella classe, in quante visite ogni controllo della coppia era vero e in
//   quante si poteva mettere.
// Se l'orario e' completo non c'e' una cella piu' avanti: stampa {"completo":1}.
// I nomi dei controlli sono quelli di passo.h.
//
// Con VISITE scrive anche in quel file il nodo di ogni visita, in ordine: un intero a 64 bit per visita (nell'ordine
// dei byte del computer). La visita i (da 1) e' all'8 * (i - 1)-esimo byte: il sito ci trova il nodo a cui saltare.

int stampa_ultima_cella(unsigned long long int seme, int classi, int ritardi, const char *file_passi,
                        const char *file_visite);

#endif //UNTITLED_ULTIMA_H

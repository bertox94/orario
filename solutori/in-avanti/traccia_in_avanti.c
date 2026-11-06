// Il controllo in avanti nella traccia: con la coppia (o il docente da solo) nella cella per prova,
// cella_bloccata_dopo_la_scelta dice se il resto e' ancora possibile. Si calcola per tutti, anche dopo un controllo
// falso: la ricerca in quel caso non ci arriva, ma la pagina lo mostra con «tutti i motivi». Si aggancia alla traccia
// della radice con traccia_controllo_extra (vedi ingresso_uscita/traccia.h).
#include "../../ingresso_uscita/traccia.h"
#include "../../ingresso_uscita/prova.h"

int cella_bloccata_dopo_la_scelta(int pos);
extern int motivo_blocco;  // 0 cella senza docenti, 1 lezioni fino a un'ora obbligatoria, 2 tutte le ore, 3 ore docente
void numeri_blocco(int *docente, int *classe, int *restano, int *possibili);
int g_giorno(int p);
int g_ora(int p);
int g_classe(int p);

static const char *nome_motivo(const int m) {
    return m == 0 ? "cella" : m == 1 ? "ore" : m == 2 ? "ore_classe" : "ore_docente";
}

int traccia_c_avanti(const int pos, const int giorno, const int ora, const int cla, const int base2d_ci,
                     const int base3d_goi, QueueItem *it, QueueItem *jt, int *fermato_a, const int k) {
    FILE *t = traccia_file();
    traccia_inizia_controllo("lascia_un_docente_alle_celle_di_oggi");
    const Prova s = metti_per_prova(pos, giorno, cla, base2d_ci, base3d_goi, it, jt);
    const int q = cella_bloccata_dopo_la_scelta(pos);
    togli_prova(&s, pos, giorno, cla, base2d_ci, base3d_goi, it, jt);
    fprintf(t, "\"cella_bloccata\":%d,\"motivo\":\"%s\"", q, nome_motivo(motivo_blocco));
    if (q >= 0) fprintf(t, ",\"giorno\":%d,\"ora\":%d,\"classe\":%d", g_giorno(q), g_ora(q), g_classe(q));
    if (q >= 0 && motivo_blocco >= 2) {  // i numeri: docente (-1 per le ore della classe), classe (-1: in tutto)
        int d, c, r, p;
        numeri_blocco(&d, &c, &r, &p);
        fprintf(t, ",\"docente\":%d,\"classe_conto\":%d,\"restano\":%d,\"possibili\":%d", d, c, r, p);
    }
    traccia_fine_controllo(q < 0, fermato_a, k);
    return q < 0;
}

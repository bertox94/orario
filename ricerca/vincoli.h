#ifndef UNTITLED_VINCOLI_H
#define UNTITLED_VINCOLI_H

// I vincoli dell'orario, come funzioni che rispondono si'/no. Sono static inline (qui nell'header) per restare
// espanse dentro la ricerca, che le chiama ad ogni nodo.

#include "../data_initialization/database_initializers.h"
#include "../orario/orario.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* ---- vincoli sullo slot / sulla classe ---- */

static inline int activities_for_this_class_and_hour(const teaching_slot *slot) {
    const int gio = slot->giorno;
    const int ora = slot->ora;
    const int cla = slot->classe;
    int ore_rimanenti = classe_info[cla].ore - classe_info[cla].svolte;
    int slot_obligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    int slot_minimi_da_riempire = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana;
    int slot_mancanti = slot_minimi_da_riempire - slot_obligatori_gia_occupati;

    if (ore_rimanenti < slot_mancanti) {
        return 0;
    }
    //se c'è almeno un ora buca, non mettere altre lezioni
    if (n_ore_buche[slot->base2d_gc + cla] > 0)
        return 0;

    return 1;
}

/* ---- regola del sabato ---- */

// Regola del sabato (ultimo giorno): se nell'ora precedente c'erano docenti con ore ancora da fare in questa classe,
// non si puo' cambiare: vanno completati subito. prof2 == -1 indica un docente che insegna da solo.
static inline int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        const teaching_slot *prev = slot_precedente(giorno, ora, orario[pos].classe);
        const int prev_prof1 = prev->insegnante1;
        const int prev_prof2 = prev->insegnante2;
        // lo slot precedente ha sempre insegnante1 != -1 se ha insegnante2 != -1 (le coppie sono scritte insieme)

        if (prev_prof1 != -1 && prev_prof2 != -1) {
            const int da_fare1 = classe_docente[base2d_ci + prev_prof1].ore - classe_docente[base2d_ci + prev_prof1].svolte > 0;
            const int da_fare2 = classe_docente[base2d_ci + prev_prof2].ore - classe_docente[base2d_ci + prev_prof2].svolte > 0;
            if (da_fare1 && da_fare2) {
                // entrambi devono esserci anche in quest'ora: possibile solo come la stessa coppia (mai da soli)
                return prof1 == prev_prof1 && prof2 == prev_prof2
                       || prof1 == prev_prof2 && prof2 == prev_prof1;
            } else if (da_fare1) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            } else if (da_fare2) {
                return prof1 == prev_prof2 || prof2 == prev_prof2;
            }
        } else if (prev_prof1 != -1) {
            if (classe_docente[base2d_ci + prev_prof1].ore - classe_docente[base2d_ci + prev_prof1].svolte > 0) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            }
        }
    }
    return 1;
}

/* ---- vincoli su un singolo docente ---- */

static inline int prof_fa_al_max_n_ore(const teaching_slot *slot, const int prof) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + prof] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

static inline int prof_fa_ore_contigue(const teaching_slot *slot, const int prof) {
    if (ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + prof]) {
        const teaching_slot *prev = slot_precedente(slot->giorno, slot->ora, slot->classe);
        return prev->insegnante1 == prof || prev->insegnante2 == prof;
    }
    return 1;
}

static inline int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

static inline int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return classe_docente[base2d_ci + p1].svolte < classe_docente[base2d_ci + p1].ore;
}

/* ---- codocenza ---- */

// Posizione, in codocenza[_svolte].ore, della coppia (p1, p2) nella classe cla (prof a indice minore prima).
// Qui in locale (e non in database_initializers.c) cosi' il compilatore la espande inline.
static inline int idx_codocenza(const int cla, const int p1, const int p2) {
    const int lo = p1 < p2 ? p1 : p2;
    const int hi = p1 < p2 ? p2 : p1;
    return cla * INSEGNANTE_SZ * INSEGNANTE_SZ + lo * INSEGNANTE_SZ + hi;
}

static inline int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, const int p1, const int p2) {
    const int i = idx_codocenza(cla, p1, p2);
    return codocenza[i].svolte < codocenza[i].ore;
}

static inline int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return classe_docente[base2d_ci + prof].alone_svolte < classe_docente[
               base2d_ci + prof].alone;
}

static inline int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, const int prof) {
    // Assuming start calculation based on your layout structure
    int start = giorno * actual_ore_giornaliere * actual_classi_sezione;

    // Fixed-size array instead of VLA (Variable Length Array).
    // Since max distinct classes allowed is n, we only ever need n elements.
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO];
    int classi_sz = 0;

    // Add the initial/current class
    classi[classi_sz++] = cla;

    for (int i = start; i < pos; i++) {
        if (orario[i].insegnante1 == prof || orario[i].insegnante2 == prof) {
            int current_class = orario[i].classe;

            // Check if the class is already recorded
            int found = 0;
            for (int j = 0; j < classi_sz; j++) {
                if (classi[j] == current_class) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                // If we already have 'max_diff_class' distinct classes and find another one, fail early
                if (classi_sz >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) {
                    return 0;
                }
                classi[classi_sz++] = current_class;
            }

            // Jump to the next hour slot (-1 to compensate for the loop's i++)
            i = start + (orario[i].ora + 1) * actual_classi_sezione - 1;
        }
    }

    return 1;
}

static inline int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= docente_info[prof].ora_entrata;
}

// Vincoli comuni a ogni docente (il primo o il secondo di una coppia) per essere messo in questo slot.
static inline int docente_idoneo(const teaching_slot *slot, const int pos, const int prof) {
    // non prima della sua ora di ingresso
    return verifica_ora_ingresso(prof, slot->ora)
           // un prof fa al max n ore in una classe
           && prof_fa_al_max_n_ore(slot, prof)
           // se un prof fa piu' ore in una classe in un giorno, devono essere contigue
           && prof_fa_ore_contigue(slot, prof)
           // ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse ogni giorno
           && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, slot->giorno, slot->classe, prof);
}


/* ---- potatura: le ore che mancano stanno ancora nel tempo rimasto? ---- */

// Controllo fatto a ogni inizio ora (vedi vincoli.c). Ritorna 0 se il ramo non puo' piu' avere soluzioni.
int ore_rimaste_stanno_nel_tempo_rimasto(int giorno, int ora);

// Elenca le lezioni (docente da solo o coppia) di ogni classe. Da chiamare una volta, dopo aver caricato i dati.
void prepara_lezioni_delle_classi(void);

// All'inizio dell'ora (giorno, ora): le lezioni che restano a ogni classe bastano per gli slot che deve riempire per
// forza (ore obbligatorie e ore rimaste)? Se no ritorna 0 e scrive la classe, l'ora fino a cui non bastano, quanti slot
// vanno riempiti e quante lezioni al massimo possono riempirli. All'inizio della settimana (0, 0) e' una verifica
// preliminare dei dati.
int lezioni_bastano_per_le_classi(int giorno, int ora, int *classe_ko, int *ora_ko, int *servono, int *possibili);

// La stessa verifica per la sola classe dello slot, dall'ora dopo lo slot. Dipende solo da quella classe, quindi si fa
// subito dopo averne deciso lo slot: cosi' una scelta sbagliata si scarta prima di provare gli slot delle altre classi.
int lezioni_della_classe_bastano_dopo(const teaching_slot *slot);

#endif //UNTITLED_VINCOLI_H

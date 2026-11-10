// I dati di prova: la scuola su cui gira ./untitled senza argomenti (i casi di test invece vengono da generatore.c).
// Giorni, ore e classi da constants/, i docenti dall'array professori (constants/professors.h), cercati per nome.

#include "database_initializers.h"
#include "../priority_queue/prio_queue.h"
#include "../constants/days.h"
#include "../constants/hours.h"
#include "../constants/professors.h"
#include "../constants/classes.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Il docente con questo nome nell'array professori (il nome deve esserci: un nome sbagliato ferma il programma).
static char *prof(const char *nome) {
    for (int i = 0; i < N_PROFESSORI; i++)
        if (!strcmp(professori[i], nome)) return professori[i];
    fprintf(stderr, "Docente sconosciuto nei dati di prova: %s\n", nome);
    abort();
}

// Giorni e ore, le lezioni classe per classe (i docenti si registrano alla prima lezione: l'ordine decide i
// loro indici e quindi l'ordine della coda a parita' di priorita'), le ore di ingresso, e il controllo delle ore.
void inizializza_dati() {
    set_giorno_settimana(Lun);
    set_giorno_settimana(Mar);
    set_giorno_settimana(Mer);
    set_giorno_settimana(Gio);
    set_giorno_settimana(Ven);
    set_giorno_settimana(Sab);

    set_ora_giornaliera(Ora_08);
    set_ora_giornaliera(Ora_09);
    set_ora_giornaliera(Ora_10);
    set_ora_giornaliera(Ora_11);
    set_ora_giornaliera(Ora_12);
    set_ora_giornaliera(Ora_13);

    set_classe_insegnante_ore(Classe_3A, prof("D. Malfoy"), 2);
    set_classe_insegnante_ore(Classe_3A, prof("Voldemort"), 3);
    set_classe_insegnante_ore(Classe_3A, prof("Itachi"), 6);
    set_classe_insegnante_ore_cod(Classe_3A, prof("Lupin"), 5, prof("G. Vermilinguo"), 3);
    set_classe_insegnante_ore(Classe_3A, prof("Brook"), 4);
    set_classe_insegnante_ore_cod(Classe_3A, prof("Ash"), 4, prof("Theoden"), 2);
    set_classe_insegnante_ore_cod(Classe_3A, prof("E. Jaeger"), 4, prof("Denethor"), 2);
    set_classe_insegnante_ore(Classe_3A, prof("R. Weasley"), 1);
    set_classe_insegnante_ore_cod(Classe_3A, prof("Mufasa"), 3, prof("M. Uchiha"), 3);

    set_classe_insegnante_ore_cod(Classe_3B, prof("Mufasa"), 5, prof("Radagast"), 4);
    set_classe_insegnante_ore(Classe_3B, prof("Simba"), 6);
    set_classe_insegnante_ore_cod(Classe_3B, prof("Scar"), 4, prof("Theoden"), 2);
    set_classe_insegnante_ore(Classe_3B, prof("D. Malfoy"), 2);
    set_classe_insegnante_ore_cod(Classe_3B, prof("E. Jaeger"), 3, prof("G. Vermilinguo"), 3);
    set_classe_insegnante_ore(Classe_3B, prof("R. Weasley"), 1);
    set_classe_insegnante_ore_cod(Classe_3B, prof("Sarabi"), 4, prof("M. Tartarughe"), 1);
    set_classe_insegnante_ore_cod(Classe_3B, prof("Ash"), 4, prof("Denethor"), 2);
    set_classe_insegnante_ore(Classe_3B, prof("Pumba"), 3);


    set_classe_insegnante_ore(Classe_3C, prof("Goten"), 3);
    set_classe_insegnante_ore(Classe_3C, prof("Trunks"), 6);
    set_classe_insegnante_ore_cod(Classe_3C, prof("Timon"), 5, prof("Jesus"), 2);
    set_classe_insegnante_ore_cod(Classe_3C, prof("Taipei"), 4, prof("M. Tartarughe"), 1);
    set_classe_insegnante_ore_cod(Classe_3C, prof("Zazu"), 3, prof("Feanor"), 2);
    set_classe_insegnante_ore(Classe_3C, prof("Doraemon"), 1);
    set_classe_insegnante_ore(Classe_3C, prof("Gohan"), 2);
    set_classe_insegnante_ore_cod(Classe_3C, prof("Rafiki"), 4, prof("Gothmog"), 2);
    set_classe_insegnante_ore_cod(Classe_3C, prof("Jasmine"), 4, prof("Morgoth"), 2);

    set_classe_insegnante_ore_cod(Classe_3D, prof("Mammolo"), 5, prof("Morgoth"), 2);
    set_classe_insegnante_ore_cod(Classe_3D, prof("Sauron"), 4, prof("Gothmog"), 2);
    set_classe_insegnante_ore_cod(Classe_3D, prof("Zazu"), 3, prof("Feanor"), 2);
    set_classe_insegnante_ore_cod(Classe_3D, prof("Aladino"), 4, prof("Sarabi"), 1);
    set_classe_insegnante_ore(Classe_3D, prof("Biancaneve"), 3);
    set_classe_insegnante_ore(Classe_3D, prof("Dotto"), 6);
    set_classe_insegnante_ore(Classe_3D, prof("Doraemon"), 1);
    set_classe_insegnante_ore_cod(Classe_3D, prof("Pisolo"), 4, prof("Gothmog"), 2);
    set_classe_insegnante_ore(Classe_3D, prof("Arwen"), 2);

    set_classe_insegnante_ore(Classe_3E, prof("Jiraya"), 4);
    set_classe_insegnante_ore(Classe_3E, prof("Orochimaru"), 6);
    set_classe_insegnante_ore_cod(Classe_3E, prof("Naruto"), 3, prof("Freezer"), 1);
    set_classe_insegnante_ore_cod(Classe_3E, prof("Cell"), 6, prof("Sasuke"), 3);
    set_classe_insegnante_ore(Classe_3E, prof("Gohan"), 2);
    set_classe_insegnante_ore_cod(Classe_3E, prof("Sandokan"), 3, prof("Jesus"), 2);
    set_classe_insegnante_ore(Classe_3E, prof("Doraemon"), 1);
    set_classe_insegnante_ore_cod(Classe_3E, prof("Vegeta"), 4, prof("Freezer"), 2);
    set_classe_insegnante_ore(Classe_3E, prof("Tsunade"), 3); //130 KHz

    set_classe_insegnante_ore(Classe_3F, prof("M. Tartarughe"), 4);
    set_classe_insegnante_ore(Classe_3F, prof("Sakura"), 6);
    set_classe_insegnante_ore_cod(Classe_3F, prof("Goku"), 3, prof("M. Bu"), 1);
    set_classe_insegnante_ore_cod(Classe_3F, prof("Chichi"), 6, prof("Bulma"), 3);
    set_classe_insegnante_ore(Classe_3F, prof("Gohan"), 2);
    set_classe_insegnante_ore_cod(Classe_3F, prof("Sandokan"), 3, prof("Jesus"), 2);
    set_classe_insegnante_ore(Classe_3F, prof("Doraemon"), 1);
    set_classe_insegnante_ore_cod(Classe_3F, prof("Vegeta"), 4, prof("Freezer"), 2);
    set_classe_insegnante_ore(Classe_3F, prof("H. Potter"), 3); //91 KHz

    set_classe_insegnante_ore(Classe_3G, prof("B. Baggins"), 2);
    set_classe_insegnante_ore(Classe_3G, prof("Lupin"), 5);
    set_classe_insegnante_ore_cod(Classe_3G, prof("S. Gamgee"), 4, prof("Morgoth"), 2);
    set_classe_insegnante_ore(Classe_3G, prof("H. Granger"), 6);
    set_classe_insegnante_ore_cod(Classe_3G, prof("Pipino"), 3, prof("Sarabi"), 2);
    set_classe_insegnante_ore_cod(Classe_3G, prof("Merry"), 4, prof("Denethor"), 3);
    set_classe_insegnante_ore(Classe_3G, prof("R. Weasley"), 1);
    set_classe_insegnante_ore(Classe_3G, prof("Elrond"), 5);
    set_classe_insegnante_ore(Classe_3G, prof("Arwen"), 2); //65 KHz

    set_classe_insegnante_ore(Classe_3K, prof("Saruman"), 4);
    set_classe_insegnante_ore(Classe_3K, prof("Gandalf"), 4);
    set_classe_insegnante_ore(Classe_3K, prof("Legolas"), 6);
    set_classe_insegnante_ore(Classe_3K, prof("Aragorn"), 3);
    set_classe_insegnante_ore(Classe_3K, prof("F. Baggins"), 6);
    set_classe_insegnante_ore(Classe_3K, prof("Arwen"), 2);
    set_classe_insegnante_ore(Classe_3K, prof("Gimli"), 2);
    set_classe_insegnante_ore(Classe_3K, prof("Glorfindel"), 4);
    set_classe_insegnante_ore(Classe_3K, prof("Galadriel"), 1); //440 Hz

    set_classe_insegnante_ore_cod(Classe_3H, prof("Taipei"), 4, prof("M. Tartarughe"), 1);
    set_classe_insegnante_ore(Classe_3H, prof("Trunks"), 6);
    set_classe_insegnante_ore_cod(Classe_3H, prof("Crillin"), 3, prof("M. Bu"), 1);
    set_classe_insegnante_ore_cod(Classe_3H, prof("Vegeta"), 6, prof("Gogeta"), 3);
    set_classe_insegnante_ore(Classe_3H, prof("Gohan"), 2);
    set_classe_insegnante_ore_cod(Classe_3H, prof("Sandokan"), 3, prof("Jesus"), 2);
    set_classe_insegnante_ore(Classe_3H, prof("Doraemon"), 1);
    set_classe_insegnante_ore_cod(Classe_3H, prof("Vegeta"), 4, prof("Janemba"), 2);
    set_classe_insegnante_ore(Classe_3H, prof("Goten"), 3); //38 KHz

    set_classe_insegnante_ore(Classe_3Q, prof("Haldir"), 6);
    set_classe_insegnante_ore_cod(Classe_3Q, prof("Mammolo"), 7, prof("Gothmog"), 4);
    set_classe_insegnante_ore_cod(Classe_3Q, prof("Zazu"), 4, prof("Gothmog"), 2);
    set_classe_insegnante_ore_cod(Classe_3Q, prof("Eomer"), 6, prof("Fingolfin"), 2);
    set_classe_insegnante_ore(Classe_3Q, prof("Eowyn"), 5);
    set_classe_insegnante_ore(Classe_3Q, prof("Celeborn"), 4);
    set_classe_insegnante_ore(Classe_3Q, prof("Lurtz"), 2);
    set_classe_insegnante_ore(Classe_3Q, prof("Galadriel"), 1); //28 KHz

    set_classe_insegnante_ore(Classe_4A, prof("Simba"), 6);
    set_classe_insegnante_ore_cod(Classe_4A, prof("T. Bombadil"), 3, prof("Theoden"), 2);
    set_classe_insegnante_ore_cod(Classe_4A, prof("T. Bombadil"), 4, prof("Denethor"), 2);
    set_classe_insegnante_ore(Classe_4A, prof("Voldemort"), 3);
    set_classe_insegnante_ore_cod(Classe_4A, prof("E. Jaeger"), 5, prof("M. Uchiha"), 5);
    set_classe_insegnante_ore(Classe_4A, prof("Saruman"), 4);
    set_classe_insegnante_ore(Classe_4A, prof("Merry"), 4);
    set_classe_insegnante_ore(Classe_4A, prof("R. Weasley"), 1);
    set_classe_insegnante_ore(Classe_4A, prof("D. Malfoy"), 2); //20 KHz

    set_classe_insegnante_ore(Classe_4B, prof("Boromir"), 4);
    set_classe_insegnante_ore_cod(Classe_4B, prof("T. Bombadil"), 3, prof("Theoden"), 2);
    set_classe_insegnante_ore(Classe_4B, prof("Lupin"), 4);
    set_classe_insegnante_ore(Classe_4B, prof("Orochimaru"), 6);
    set_classe_insegnante_ore_cod(Classe_4B, prof("Scar"), 5, prof("Radagast"), 5);
    set_classe_insegnante_ore_cod(Classe_4B, prof("Ash"), 4, prof("Denethor"), 2);
    set_classe_insegnante_ore(Classe_4B, prof("Pumba"), 3);
    set_classe_insegnante_ore(Classe_4B, prof("R. Weasley"), 1);
    set_classe_insegnante_ore(Classe_4B, prof("D. Malfoy"), 2); //23 Khz

    set_classe_insegnante_ore(Classe_4C, prof("Pipino"), 4);
    set_classe_insegnante_ore(Classe_4C, prof("Faramir"), 6);
    set_classe_insegnante_ore(Classe_4C, prof("Biancaneve"), 3);
    set_classe_insegnante_ore(Classe_4C, prof("Rafiki"), 5);
    set_classe_insegnante_ore(Classe_4C, prof("Jasmine"), 5);
    set_classe_insegnante_ore(Classe_4C, prof("Barbalbero"), 6);
    set_classe_insegnante_ore(Classe_4C, prof("Galadriel"), 1);
    set_classe_insegnante_ore(Classe_4C, prof("Gohan"), 2); //20 KHz

    set_classe_insegnante_ore(Classe_4D, prof("Rafiki"), 6);
    set_classe_insegnante_ore(Classe_4D, prof("Boromir"), 3);
    set_classe_insegnante_ore(Classe_4D, prof("Aladino"), 4);
    set_classe_insegnante_ore(Classe_4D, prof("S. Gamgee"), 5);
    set_classe_insegnante_ore(Classe_4D, prof("Isildur"), 6);
    set_classe_insegnante_ore(Classe_4D, prof("Zazu"), 5);
    set_classe_insegnante_ore(Classe_4D, prof("R. Weasley"), 1);
    set_classe_insegnante_ore(Classe_4D, prof("Gohan"), 2); //18 KHz


    set_classe_insegnante_ore(Classe_4E, prof("Jiraya"), 4);
    set_classe_insegnante_ore(Classe_4E, prof("H. Granger"), 6);
    set_classe_insegnante_ore(Classe_4E, prof("Naruto"), 7);
    set_classe_insegnante_ore(Classe_4E, prof("Cell"), 6);
    set_classe_insegnante_ore(Classe_4E, prof("MC. Granitt"), 2);
    set_classe_insegnante_ore(Classe_4E, prof("Sandokan"), 3);
    set_classe_insegnante_ore(Classe_4E, prof("Doraemon"), 1);
    set_classe_insegnante_ore(Classe_4E, prof("Tsunade"), 3);

    set_classe_insegnante_ore(Classe_5E, prof("Jiraya"), 3);
    set_classe_insegnante_ore(Classe_5E, prof("Orochimaru"), 6);
    set_classe_insegnante_ore(Classe_5E, prof("Naruto"), 4);
    set_classe_insegnante_ore(Classe_5E, prof("Cell"), 6);
    set_classe_insegnante_ore(Classe_5E, prof("Gohan"), 2);
    set_classe_insegnante_ore(Classe_5E, prof("A. Silente"), 4);
    set_classe_insegnante_ore(Classe_5E, prof("Hagrid"), 3);
    set_classe_insegnante_ore(Classe_5E, prof("Tsunade"), 3);
    set_classe_insegnante_ore(Classe_5E, prof("Doraemon"), 1);


    // le ore di ingresso: chi entra dopo la prima ora
    ora_di_entrata_prof[p_insegnante(prof("Naruto"))] = 1;
    ora_di_entrata_prof[p_insegnante(prof("Cell"))] = 1;
    ora_di_entrata_prof[p_insegnante(prof("Vegeta"))] = 2;


    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(ore_classi[i] <= actual_giorni_settimana * actual_ore_giornaliere);
        assert(ore_classi[i] >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
    }
}

//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_PROFESSORS_H
#define UNTITLED_PROFESSORS_H

// I docenti dei dati di prova, in un elenco solo: X(nome nel codice, nome stampato). Dall'elenco vengono l'enum degli
// indici (Goku = 0, Vegeta = 1, ...) e l'array professori[] dei nomi, nello stesso ordine: professori[Goku] e' "Goku".
// Nelle lezioni di inizializza_dati() si scrive professori[Goku]. Per aggiungere un docente basta una riga qui.
#define PROFESSORI(X) \
    X(Goku, "Goku") \
    X(Vegeta, "Vegeta") \
    X(Junior, "Junior") \
    X(Crillin, "Crillin") \
    X(Freezer, "Freezer") \
    X(Cell, "Cell") \
    X(MBu, "M. Bu") \
    X(Bulma, "Bulma") \
    X(Chichi, "Chichi") \
    X(Janemba, "Janemba") \
    X(Gogeta, "Gogeta") \
    X(Taipei, "Taipei") \
    X(MTartarughe, "M. Tartarughe") \
    X(Trunks, "Trunks") \
    X(Gohan, "Gohan") \
    X(Goten, "Goten") \
    X(Sandokan, "Sandokan") \
    X(Doraemon, "Doraemon") \
    X(Jesus, "Jesus") \
    X(Naruto, "Naruto") \
    X(Sasuke, "Sasuke") \
    X(Tsunade, "Tsunade") \
    X(Orochimaru, "Orochimaru") \
    X(Jiraya, "Jiraya") \
    X(Sakura, "Sakura") \
    X(HPotter, "H. Potter") \
    X(Piton, "Piton") \
    X(RWeasley, "R. Weasley") \
    X(HGranger, "H. Granger") \
    X(MCGranitt, "MC. Granitt") \
    X(ASilente, "A. Silente") \
    X(Hagrid, "Hagrid") \
    X(DMalfoy, "D. Malfoy") \
    X(Voldemort, "Voldemort") \
    X(Itachi, "Itachi") \
    X(Lupin, "Lupin") \
    X(Brook, "Brook") \
    X(Ash, "Ash") \
    X(EJaeger, "E. Jaeger") \
    X(MUchiha, "M. Uchiha") \
    X(Mufasa, "Mufasa") \
    X(Simba, "Simba") \
    X(Nala, "Scar") \
    X(Sarabi, "Sarabi") \
    X(Pumba, "Pumba") \
    X(Timon, "Timon") \
    X(Rafiki, "Rafiki") \
    X(Zazu, "Zazu") \
    X(Jasmine, "Jasmine") \
    X(Aladino, "Aladino") \
    X(Biancaneve, "Biancaneve") \
    X(Dotto, "Dotto") \
    X(Pisolo, "Pisolo") \
    X(Mammolo, "Mammolo") \
    X(Sauron, "Sauron") \
    X(BBaggins, "B. Baggins") \
    X(SGamgee, "S. Gamgee") \
    X(Merry, "Merry") \
    X(Pipino, "Pipino") \
    X(Elrond, "Elrond") \
    X(Arwen, "Arwen") \
    X(Gandalf, "Gandalf") \
    X(Saruman, "Saruman") \
    X(Legolas, "Legolas") \
    X(FBaggins, "F. Baggins") \
    X(Aragorn, "Aragorn") \
    X(Gimli, "Gimli") \
    X(Glorfindel, "Glorfindel") \
    X(Galadriel, "Galadriel") \
    X(Haldir, "Haldir") \
    X(Eomer, "Eomer") \
    X(Eowyn, "Eowyn") \
    X(Celeborn, "Celeborn") \
    X(Lurtz, "Lurtz") \
    X(TBombadil, "T. Bombadil") \
    X(Boromir, "Boromir") \
    X(Faramir, "Faramir") \
    X(Barbalbero, "Barbalbero") \
    X(Isildur, "Isildur") \
    X(Denethor, "Denethor") \
    X(GVermilinguo, "G. Vermilinguo") \
    X(Radagast, "Radagast") \
    X(Theoden, "Theoden") \
    X(Morgoth, "Morgoth") \
    X(Gothmog, "Gothmog") \
    X(Feanor, "Feanor") \
    X(Fingolfin, "Fingolfin")

#define PROFESSORE_INDICE(id, nome) id,
#define PROFESSORE_NOME(id, nome) nome,
enum { PROFESSORI(PROFESSORE_INDICE) N_PROFESSORI };
static char professori[N_PROFESSORI][MAX_LEN_WORDS] = {PROFESSORI(PROFESSORE_NOME)};

#endif //UNTITLED_PROFESSORS_H

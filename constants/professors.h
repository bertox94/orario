//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_PROFESSORS_H
#define UNTITLED_PROFESSORS_H

// I docenti dei dati di prova (inizializza_dati), come NOMI_DOCENTI_CASUALI per i casi di test: un array di nomi.
// Nelle lezioni un docente si scrive con il suo nome, prof("Goku"), che lo cerca qui.
static char professori[][MAX_LEN_WORDS] = {
    "Goku", "Vegeta", "Junior", "Crillin", "Freezer", "Cell", "M. Bu", "Bulma", "Chichi", "Janemba", "Gogeta", "Taipei",
    "M. Tartarughe", "Trunks", "Gohan", "Goten", "Sandokan", "Doraemon", "Jesus", "Naruto", "Sasuke", "Tsunade",
    "Orochimaru", "Jiraya", "Sakura", "H. Potter", "Piton", "R. Weasley", "H. Granger", "MC. Granitt", "A. Silente",
    "Hagrid", "D. Malfoy", "Voldemort", "Itachi", "Lupin", "Brook", "Ash", "E. Jaeger", "M. Uchiha", "Mufasa", "Simba",
    "Scar", "Sarabi", "Pumba", "Timon", "Rafiki", "Zazu", "Jasmine", "Aladino", "Biancaneve", "Dotto", "Pisolo",
    "Mammolo", "Sauron", "B. Baggins", "S. Gamgee", "Merry", "Pipino", "Elrond", "Arwen", "Gandalf", "Saruman",
    "Legolas", "F. Baggins", "Aragorn", "Gimli", "Glorfindel", "Galadriel", "Haldir", "Eomer", "Eowyn", "Celeborn",
    "Lurtz", "T. Bombadil", "Boromir", "Faramir", "Barbalbero", "Isildur", "Denethor", "G. Vermilinguo", "Radagast",
    "Theoden", "Morgoth", "Gothmog", "Feanor", "Fingolfin",
};
#define N_PROFESSORI ((int) (sizeof professori / sizeof professori[0]))

#endif //UNTITLED_PROFESSORS_H

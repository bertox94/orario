#ifndef UNTITLED_NOMI_DOCENTI_H
#define UNTITLED_NOMI_DOCENTI_H

// Nomi dei docenti dei dati casuali (generatore.c): personaggi di fantasia come quelli dei dati del programma
// (constants/professors.h), ma tutti diversi da quelli. Il docente d (da 0) si chiama NOMI_DOCENTI_CASUALI[d]; oltre la
// fine dell'elenco D%03d. Sono mescolati tra le varie saghe, cosi' anche i primi di ogni caso sono vari.
// Solo ASCII e al massimo 16 caratteri: stanno nelle celle della griglia stampata (MAX_STR_SZ_IN_DISPLAY).
// Cambiare l'elenco cambia solo i nomi, non la ricerca (che lavora sugli indici dei docenti).

static const char *const NOMI_DOCENTI_CASUALI[] = {
    "Bardack", "Kakashi", "Rufy", "Sirius", "Thranduil", "Topolino", "Zelda", "Radish", "Gaara", "Zoro", "N. Paciock",
    "Thorin", "Paperino", "Link", "Nappa", "Hinata", "Nami", "L. Lovegood", "Balin", "Pippo", "Ganondorf", "Zarbon",
    "Neji", "Usopp", "G. Weasley", "Dwalin", "Pluto", "Impa", "Dodoria", "Rock Lee", "Sanji", "F. Weasley", "Fili",
    "Minni", "Mario", "Ginew", "Tenten", "Chopper", "A. Weasley", "Kili", "Paperina", "Luigi", "Recoome", "Shikamaru",
    "N. Robin", "M. Weasley", "Bombur", "Paperone", "Peach", "Jeeth", "Choji", "Franky", "P. Weasley", "Bofur", "Qui",
    "Bowser", "Burter", "Ino", "Jinbe", "C. Diggory", "Oin", "Quo", "Yoshi", "Guldo", "Kiba", "Shanks", "C. Chang",
    "Gloin", "Qua", "Wario", "C-17", "Shino", "Barbabianca", "V. Krum", "Dori", "Gastone", "Sonic", "C-18", "Kurenai",
    "Ace", "F. Delacour", "Nori", "Archimede", "Tails", "C-16", "Asuma", "Sabo", "Lumacorno", "Ori", "Basettoni",
    "Knuckles", "Dr. Gero", "Gai", "Garp", "Allock", "Bard", "Gambadilegno", "Eggman", "Kaioh", "Minato", "Crocodile",
    "Raptor", "Smaug", "Ariel", "Aslan", "Dende", "Kushina", "Mihawk", "Umbridge", "Beorn", "Ursula", "Tumnus",
    "Yamcha", "Obito", "Kaido", "Moody", "Azog", "Sebastian", "Jadis", "Tensing", "Hashirama", "Big Mom", "Tonks",
    "Bolg", "Tritone", "Caspian", "Jiaozi", "Tobirama", "Doflamingo", "Kingsley", "Gollum", "Belle", "Nobita", "Muten",
    "Hiruzen", "T. Law", "L. Malfoy", "Shelob", "Bestia", "Shizuka", "Oolong", "Danzo", "Hancock", "N. Malfoy", "Ugluk",
    "Gaston", "Gian", "Puar", "Pain", "Buggy", "Bellatrix", "Grishnakh", "Lumiere", "Suneo", "Lunch", "Konan", "Smoker",
    "Codaliscia", "Gorbag", "Cenerentola", "Usagi", "Videl", "Kisame", "Akainu", "Dobby", "Shagrat", "Malefica", "Ami",
    "M. Satan", "Deidara", "Aokiji", "Kreacher", "Manwe", "Aurora", "Rei", "Pan", "Sasori", "Kizaru", "Fierobecco",
    "Varda", "Peter Pan", "Makoto", "Bra", "Hidan", "Gazza", "Ulmo", "Uncino", "Minako", "Beerus", "Kakuzu", "Vitious",
    "Aule", "Spugna", "Luna", "Whis", "Kabuto", "Sprite", "Yavanna", "Trilli", "Seiya", "Hit", "Yamato", "Cooman",
    "Orome", "Wendy", "Shiryu", "Jiren", "Sai", "Ollivander", "Mandos", "Mowgli", "Hyoga", "Broly", "Temari",
    "Grindelwald", "Tulkas", "Baloo", "Shun", "Cooler", "Kankuro", "Nagini", "Melian", "Bagheera", "Ikki", "Zamasu",
    "Killer Bee", "Aragog", "Thingol", "Shere Khan", "Oscar", "Tapion", "Haku", "Firenze", "Luthien", "Kaa", "Andre",
    "Bojack", "Zabuza", "Beren", "Pinocchio", "Holly", "Kibith", "Iruka", "Turin", "Geppetto", "Benji", "Shenron",
    "Konohamaru", "Hurin", "Mangiafuoco", "M. Lenders", "Turgon", "Lucignolo", "E. Elric", "Finrod", "Dumbo",
    "A. Elric", "Maedhros", "Bambi", "Mustang", "Earendil", "Tamburino", "Mikasa", "Elwing", "Robin Hood", "Armin",
    "Cirdan", "Little John", "Levi", "Gil-galad", "Frate Tuck", "Erwin", "Elendil", "Sir Biss", "Hange", "Celebrimbor",
    "Merlino", "Ryuk", "Glaurung", "Semola", "Ichigo", "Ungoliant", "Maga Mago", "Rukia", "Elladan", "Ercole", "Renji",
    "Elrohir", "Ade", "Aizen", "Imrahil", "Filottete", "Byakuya", "Beregond", "Megara", "Gon", "Gamling", "Mulan",
    "Killua", "Mushu", "Kurapika", "Esmeralda", "Leorio", "Quasimodo", "Hisoka", "Frollo", "Conan", "Tarzan", "Kogoro",
    "Stitch", "Jigen", "Lilo", "Goemon", "Rapunzel", "Fujiko", "Gothel", "Zenigata", "Elsa", "Actarus", "Olaf", "Alcor",
    "Kristoff", "Venusia", "Moana", "Heidi", "Maui", "Rottenmeier", "Jafar", "Asterix", "Iago", "Obelix", "Abu",
    "Panoramix", "Genio", "Idefix", "Cruella", "Gargamella", "Pongo", "Puffetta", "Brontolo", "Birba", "Eolo",
    "Gongolo", "Cucciolo", "Grimilde"
};

#define N_NOMI_DOCENTI_CASUALI ((int) (sizeof NOMI_DOCENTI_CASUALI / sizeof NOMI_DOCENTI_CASUALI[0]))

#endif //UNTITLED_NOMI_DOCENTI_H

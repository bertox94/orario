# Compila il solver: make. Il sito lo compila da solo (sito/solver.py), questo serve per usarlo a mano.
# con tutti gli avvisi del compilatore accesi: il codice del solver deve compilare senza nessuno
CARTELLE = scuola generatore solver verifica passi spiegazioni programma
SORGENTI = $(foreach c,$(CARTELLE),$(wildcard $(c)/*.c))
orario: $(SORGENTI) $(foreach c,$(CARTELLE),$(wildcard $(c)/*.h))
	gcc -O2 -std=gnu11 -Wall -Wextra $(SORGENTI) -o orario

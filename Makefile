# Compila il solver: make. Il sito lo compila da solo (sito/solver.py), questo serve per usarlo a mano.
# con tutti gli avvisi del compilatore accesi: il codice del solver deve compilare senza nessuno
orario: solver/*.c solver/*.h
	gcc -O2 -std=gnu11 -Wall -Wextra solver/*.c -o orario

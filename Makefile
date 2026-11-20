# Compila il solver: make. Il sito lo compila da solo (sito/solver.py), questo serve per usarlo a mano.
orario: solver/*.c solver/*.h
	gcc -O2 -std=gnu11 -Wall -Wextra solver/*.c -o orario

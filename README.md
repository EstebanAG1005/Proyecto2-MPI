# Proyecto2-MPI

Para compilar el programa partea_3.c

gcc -w partea_3.c -o partea_3 -lcrypto

Para correr el programa 

./partea_3

# parte de bruteforce

Para compilar el programa partea_3.c

gcc -w partea_3.c -o partea_3 -lcrypto

Para correr el programa 

./partea_3

Compilar

mpicc -w -o bruteforce bruteforce.c -lssl -lcrypto

Correr el progama 

mpirun -n [numero de procesos] ./bruteforce
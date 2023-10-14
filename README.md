# Proyecto2-MPI

## Para compilar el programa partea_3.c

```
gcc -w partea_3.c -o partea_3 -lcrypto
```

## Para correr el programa

```
./partea_3*
```

## Parte de bruteforce

### Compilar

```
mpicc -w -o bruteforce bruteforce.c -lssl -lcrypto
```

### Correr el progama 

```
mpirun -n [numero de procesos] ./bruteforce
```


## Naive

### Compilar

```
mpicc -w -o Naive Naive.c -lssl -lcrypto
```

### Correr el progama 

```
mpirun -n [numero de procesos] ./Native input.txt [Llave]
```

## Saltos

### Compilar

```
mpicc -w -o Saltos Saltos.c -lssl -lcrypto
```

### Correr el progama 

```
mpirun -n [numero de procesos] ./Saltos input.txt [Llave]
```

## Random

### Compilar

```
mpicc -w -o Random Random.c -lssl -lcrypto
```

### Correr el progama 

```
mpirun -n [numero de procesos] ./Random input.txt [Llave]
```

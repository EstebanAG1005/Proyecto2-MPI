/**
 * El código es una implementación paralela de un ataque de fuerza bruta en un archivo cifrado con DES para encontrar la
 * clave de cifrado.
 *
 * @param key El parámetro "key" es la clave privada utilizada para el cifrado y descifrado. Es un valor entero largo
 * que se pasa como argumento de línea de comandos al ejecutar el programa.
 * @param ciph La variable `ciph` es un arreglo de caracteres que representa el texto cifrado. Se pasa a la función `tryKey`
 * para verificar si una clave específica puede descifrar el texto con éxito.
 * @param len La variable `len` representa la longitud del texto cifrado que se está pasando a la función `tryKey`.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <stdint.h>

#define WORK_TAG 1
#define STOP_TAG 2
#define CHUNK_SIZE 10000 // Definir el tamaño de los bloques de claves a probar

// Función para desencriptar usando DES
void decrypt(uint64_t key, char *ciph, int len)
{
    DES_key_schedule ks;
    DES_cblock k;
    memcpy(k, &key, 8);                                                              // Copiar la llave en el bloque k
    DES_set_odd_parity(&k);                                                          // Establecer paridad impar
    DES_set_key_checked(&k, &ks);                                                    // Establecer llave para la encriptación
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_DECRYPT); // Desencriptar en modo ECB
}

// Función para encriptar usando DES
void encrypt(uint64_t key, char *ciph)
{
    DES_key_schedule ks;
    DES_cblock k;
    memcpy(k, &key, 8);                                                              // Copiar la llave en el bloque k
    DES_set_odd_parity(&k);                                                          // Establecer paridad impar
    DES_set_key_checked(&k, &ks);                                                    // Establecer llave para la encriptación
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_ENCRYPT); // Encriptar en modo ECB
}

// Función para añadir relleno y garantizar bloques de 8 bytes
/**
 * La función `addPadding` añade relleno a un texto dado para asegurar que su tamaño sea múltiplo de 8.
 *
 * @param text Un puntero a un arreglo de caracteres sin firmar que representa el texto original.
 * @param size El parámetro "size" es un puntero a una variable que almacena el tamaño del arreglo "text".
 *
 * @return un puntero a un carácter sin firmar, que representa el texto con relleno.
 */
unsigned char *addPadding(unsigned char *text, size_t *size)
{
    int padding_size = 8 - (*size % 8);
    unsigned char *padded_text = malloc(*size + padding_size + 1);
    memcpy(padded_text, text, *size);
    for (int i = 0; i < padding_size; i++)
    {
        padded_text[*size + i] = padding_size;
    }
    padded_text[*size + padding_size] = '\0';
    *size = *size + padding_size;
    return padded_text;
}

/**
 * La función lee un archivo, cifra su contenido usando una clave dada y devuelve el texto cifrado.
 *
 * @param filename El parámetro "filename" es una cadena que representa el nombre del archivo que se va a leer
 * y cifrar.
 * @param key El parámetro "key" es un entero largo que se utiliza para la cifra. Se usa como una clave secreta
 * para realizar operaciones de cifrado en el contenido del archivo.
 *
 * @return un puntero al texto cifrado.
 */
// Leer archivo, encriptarlo y devolver el texto encriptado
char *read_file_and_encrypt(const char *filename, long key)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_contents = (char *)malloc(file_size + 1);
    if (!file_contents)
    {
        perror("Error reservando memoria");
        exit(EXIT_FAILURE);
    }

    fread(file_contents, 1, file_size, file);
    fclose(file);

    file_contents[file_size] = '\0';

    // Agregar padding al texto
    unsigned char *padded_text = addPadding((unsigned char *)file_contents, &file_size);
    free(file_contents);

    // Encriptar el texto con padding
    encrypt(key, (char *)padded_text);

    return (char *)padded_text;
}

// Palabra clave a buscar en el texto descifrado para determinar si el cifrado fue exitoso
char search[] = "Esta";

// Probar una llave específica y verificar si el texto desencriptado contiene la palabra clave
int tryKey(uint64_t key, char *ciph, int len)
{
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr((char *)temp, search) != NULL;
}

/**
 * La función anterior es un programa paralelo que busca una clave para descifrar un texto cifrado utilizando la biblioteca MPI
 * (Message Passing Interface).
 *
 * @param argc La variable `argc` representa el número de argumentos de línea de comandos pasados al
 * programa. En este caso, es el número total de argumentos, incluyendo el nombre del programa en sí.
 * @param argv - argv[0]: El nombre del archivo ejecutable.
 */
int main(int argc, char *argv[])
{

    // Verificar argumentos
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <nombre_archivo> <llave_privada>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    long the_key = strtol(argv[2], NULL, 10);
    double start_time, end_time;
    int N, id;
    uint64_t upper = (1ULL << 56);
    uint64_t mylower, myupper;
    MPI_Status st;
    MPI_Request req;

    char *eltexto = read_file_and_encrypt(filename, the_key);
    int ciphlen = strlen(eltexto);
    MPI_Comm comm = MPI_COMM_WORLD;

    char cipher[ciphlen + 1];
    memcpy(cipher, eltexto, ciphlen);
    cipher[ciphlen] = 0;

    // Inicializar MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);
    start_time = MPI_Wtime();

    long found = 0L;
    int ready = 0;

    // Ajustar rangos de búsqueda de llaves
    mylower = id;
    myupper = upper;

    // Iniciar recepción no bloqueante
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    // Búsqueda de llave
    for (uint64_t i = mylower; i < myupper; i += N)
    {
        MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
        if (ready)
            break;

        if (tryKey(i, cipher, ciphlen))
        {
            found = i;
            for (int node = 0; node < N; node++)
            {
                MPI_Send(&found, 1, MPI_LONG, node, 0, comm);
            }
            break;
        }
    }

    // Proceso principal imprime el resultado
    if (id == 0)
    {
        MPI_Wait(&req, &st);
        decrypt(found, cipher, ciphlen);
        printf("Se encontro la llave: %li\n\n", found);
        printf("Mensaje desencriptado: \n");
        printf("%s\n", cipher);
    }
    printf("Proceso %d terminando\n", id);

    end_time = MPI_Wtime();
    double elapsed_time = end_time - start_time;

    double max_elapsed_time;
    MPI_Reduce(&elapsed_time, &max_elapsed_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

    if (id == 0)
    {
        printf("Tiempo total de ejecucion: %f s\n", max_elapsed_time);
    }

    // Finalizar entorno MPI
    MPI_Finalize();
    free(eltexto);
}

/**
 * The code is a parallel implementation of a brute-force attack on a DES encrypted file to find the
 * encryption key.
 *
 * @param key The "key" parameter is the private key used for encryption and decryption. It is a long
 * integer value that is passed as a command line argument when running the program.
 * @param ciph The variable `ciph` is a character array that represents the encrypted text. It is
 * passed to the `tryKey` function to check if a specific key can decrypt the text successfully.
 * @param len The variable `len` represents the length of the ciphertext (encrypted text) that is being
 * passed to the `tryKey` function.
 */
/**
 * The above code is a parallel implementation of a brute-force attack on a DES encrypted file to find
 * the encryption key.
 *
 * @param key The "key" parameter is the private key used for encryption and decryption. It is a long
 * integer value that is passed as a command line argument when running the program.
 * @param ciph The variable `ciph` is a character array that represents the encrypted text. It is
 * passed to the `tryKey` function to check if a specific key can decrypt the text successfully.
 * @param len The variable `len` represents the length of the ciphertext (encrypted text) that is being
 * passed to the `tryKey` function.
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

// Función para añadir padding y garantizar bloques de 8 bytes
/**
 * The function `addPadding` adds padding to a given text to ensure its size is a multiple of 8.
 *
 * @param text A pointer to an array of unsigned characters representing the original text.
 * @param size The "size" parameter is a pointer to a variable that stores the size of the "text"
 * array.
 *
 * @return a pointer to an unsigned char, which represents the padded text.
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
 * The function reads a file, encrypts its contents using a given key, and returns the encrypted text.
 *
 * @param filename The filename parameter is a string that represents the name of the file to be read
 * and encrypted.
 * @param key The "key" parameter is a long integer that is used for encryption. It is used as a secret
 * key to perform encryption operations on the file contents.
 *
 * @return a pointer to the encrypted text.
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
 * The above function is a parallel program that searches for a key to decrypt a cipher text using MPI
 * (Message Passing Interface) library.
 *
 * @param argc The variable `argc` represents the number of command-line arguments passed to the
 * program. In this case, it is the total number of arguments including the program name itself.
 * @param argv - argv[0]: The name of the executable file
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

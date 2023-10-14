
/**
 * @file Random.c
 * @author Esteban Aldana
 * @author Jessica Ortiz
 * @author Gabriel Vicente
 * @brief Fuerza bruta con saltos y random
 * @date 2023-10-13
 * @copyright Copyright (c) 2023
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <sys/time.h>
/**
 * La función descifra un texto cifrado determinado utilizando el algoritmo DES con una clave específica.
 *
 * @param key El parámetro clave es un número entero largo que representa la clave de cifrado.
 * @param ciph El parámetro `ciph` es un puntero a una matriz de caracteres sin signo, que representa
 * el texto cifrado que necesita ser descifrado. La longitud del texto cifrado está especificada por `len`
 * parámetro.
 * @param len El parámetro "len" representa la longitud del texto cifrado en bytes.
 */
void decrypt(long key, unsigned char *ciph, int len){
    DES_cblock k;
    for(int i=0; i<8; ++i){
        k[i] = (key >> (8*(7-i))) & 0xFF;
    }
    DES_key_schedule ks;
    DES_set_odd_parity(&k);
    DES_set_key_checked(&k, &ks);
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_DECRYPT);
}
/**
 * La función cifra un mensaje determinado utilizando el algoritmo de cifrado DES con una clave específica.
 *
 * @param key El parámetro "key" es un número entero largo que representa la clave de cifrado. Es usado para
 * generar la programación de claves DES, que es un conjunto de subclaves utilizadas en el proceso de cifrado.
 * @param ciph El parámetro `ciph` es un puntero a una matriz de caracteres sin firmar, que representa
 * los datos de entrada que se cifrarán.
 * @param len El parámetro "len" representa la longitud de los datos de entrada en bytes.
 */

void encrypt(long key, unsigned char *ciph, int len){
    DES_cblock k;
    for(int i=0; i<8; ++i){
        k[i] = (key >> (8*(7-i))) & 0xFF;
    }
    DES_key_schedule ks;
    DES_set_odd_parity(&k);
    DES_set_key_checked(&k, &ks);
    DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_ENCRYPT);
}

/**
 * La función `addPadding` agrega relleno a un texto determinado para que su tamaño sea múltiplo de 8.
 *
 * @param text El parámetro "texto" es un puntero a una matriz de caracteres sin firmar, que representa
 * el texto original que necesita ser completado.
 * @param size El parámetro "tamaño" es un puntero a una variable que almacena el tamaño del "texto"
 * matriz.
 *
 * @return un puntero a un carácter sin firmar, que representa el texto acolchado.
 */
unsigned char* addPadding(unsigned char* text, size_t* size) {
    int padding_size = 8 - (*size % 8);
    unsigned char* padded_text = malloc(*size + padding_size + 1);
    memcpy(padded_text, text, *size);
    for (int i = 0; i < padding_size; i++) {
        padded_text[*size + i] = padding_size;
    }
    padded_text[*size + padding_size] = '\0';
    *size = *size + padding_size;
    return padded_text;
}

/**
 * La función lee un archivo, agrega relleno al contenido, cifra los datos usando una clave determinada y
 * devuelve los datos cifrados.
 *
 * @param filename El parámetro filename es una cadena que representa el nombre del archivo que se va a leer
 * y cifrado.
 * @param key El parámetro "key" es un número entero largo que se utiliza para el cifrado. Se utiliza como secreto.
 * clave para realizar el cifrado de los datos de texto sin formato.
 * @param cipher El parámetro `cipher` es un puntero a un puntero de carácter sin firmar. Se utiliza para almacenar
 * los datos cifrados leídos del archivo.
 * @param cipher_len cipher_len es un puntero a una variable size_t. Se utiliza para almacenar la longitud de
 * el texto cifrado después del cifrado.
 */
void readFileAndEncrypt(const char *filename, long key, unsigned char **cipher, size_t* cipher_len) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fseek(file, 0, SEEK_END);
    size_t file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *plain = malloc(file_len + 1);
    fread(plain, 1, file_len, file);
    plain[file_len] = '\0';
    fclose(file);

    plain = addPadding(plain, &file_len);

    *cipher = malloc(file_len + 1);
    memcpy(*cipher, plain, file_len);
    (*cipher)[file_len] = '\0';

    free(plain);
    *cipher_len = file_len;

    encrypt(key, *cipher, *cipher_len);
}
/**
 * La función `cleanAndPrintText` toma una cadena de caracteres e imprime solo los imprimibles
 * Caracteres ASCII, reemplazando los caracteres no imprimibles por un punto.
 *
 * @param text El parámetro "texto" es un puntero a una matriz de caracteres sin firmar, que representa
 * el texto a limpiar e imprimir.
 * @param len El parámetro `len` representa la longitud de la matriz `text`. Especifica el número de
 * elementos de la matriz que deben procesarse.
 */
void cleanAndPrintText(unsigned char *text, size_t len) {
    for(size_t i = 0; i < len; i++) {

        if(text[i] >= 32 && text[i] <= 126) {
            printf("%c", text[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}
/**
 * La función `tryKey` toma una clave, un texto cifrado, su longitud, una cadena de búsqueda y un búfer para
 * texto descifrado y devuelve si el texto descifrado contiene la cadena de búsqueda.
 *
 * @param key El parámetro "key" es un número entero largo que representa la clave de cifrado utilizada para descifrar
 * la matriz "ciph".
 * @param ciph El parámetro `ciph` es un puntero a una matriz de caracteres sin signo, que representa
 * el texto cifrado.
 * @param len El parámetro "len" representa la longitud del texto cifrado (ciph) en bytes.
 * @param search El parámetro "search" es un puntero a una cadena terminada en nulo que desea
 * buscar en el texto descifrado.
 * @param decrypted_text El parámetro `decrypted_text` es un puntero a una matriz de caracteres sin firmar que
 * representa el texto descifrado.
 *
 * @return un valor booleano que indica si la cadena de búsqueda se encuentra en el texto descifrado.
 */

int tryKey(long key, unsigned char *ciph, int len, const char *search, unsigned char *decrypted_text){
    memcpy(decrypted_text, ciph, len);
    decrypted_text[len] = 0;
    decrypt(key, decrypted_text, len);
    
    int padding_size = decrypted_text[len - 1];
    if (padding_size >= 1 && padding_size <= 8) {
        len -= padding_size;
        decrypted_text[len] = '\0';
    }
    
    return strstr((char *)decrypted_text, search) != NULL;
}

/* La línea `char search[] = "es una prueba de";` está declarando e inicializando una matriz de caracteres llamada
`buscar`. La matriz se está inicializando con la cadena "es una prueba de". Esta cadena se utiliza como
la palabra clave a buscar en el texto descifrado. */
char search[] = "es una prueba de";
/**
 * La función anterior es una implementación paralela de un ataque de fuerza bruta para descifrar un cifrado determinado.
 * texto usando una palabra clave específica.
 *
 * @param argc La variable `argc` representa el número de argumentos de línea de comando pasados ​​al
 * programa. Incluye el nombre del programa en sí como primer argumento.
 * @param argv El parámetro `argv` es una matriz de cadenas que representa los argumentos de la línea de comandos
 *pasado al programa. En este caso, `argv[0]` es el nombre del programa en sí, `argv[1]` es el
 * nombre del archivo cifrado y `argv[2]`
 *
 * @return La función principal devuelve un valor entero de 0.
 */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <encrypted_file> <keyword>\n", argv[0]);
        return 1;
    }

    unsigned char *cipher_text;
    size_t cipher_len;
    FILE *file = fopen(argv[1], "rb");

    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return 1;
    }

    double start_time;
    fseek(file, 0, SEEK_END);
    cipher_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    cipher_text = malloc(cipher_len + 1);
    fread(cipher_text, 1, cipher_len, file);
    fclose(file);
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long upper_limit = (1L << 56) - 1;
    long key;
    int found = 0;
    unsigned char *decrypted_text = malloc(cipher_len + 1);

    long keys_per_node = (upper_limit + 1) / size;
    long start_key = rank * keys_per_node;
    long end_key = (rank == size - 1) ? upper_limit : (rank + 1) * keys_per_node - 1;

    if (rank == 0) {
        printf("Procesando...\n");
        start_time = MPI_Wtime(); 
    }
    while(found == 0) {

        long random_key = rand() % (end_key + 1)+start_key;

        found = tryKey(random_key, cipher_text, cipher_len, search, decrypted_text);

        if (found != 0) {
            printf("Key found by process %d: %ld\n", rank, random_key);
            printf("Encrypted message: ");
            cleanAndPrintText(cipher_text, cipher_len);
            printf("Decrypted message: ");
            cleanAndPrintText(decrypted_text, cipher_len);

            for (int dest = 0; dest < size; dest++) {
                if (dest != rank) {
                    MPI_Send(&key, 1, MPI_LONG, dest, 0, MPI_COMM_WORLD);
                }
            }
            break;
        }

        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &found, &status);
        if (found) {

            long received_key;
            MPI_Recv(&received_key, 1, MPI_LONG, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            break;
        }
    }

    free(cipher_text);
    free(decrypted_text);

    if (rank == 0){
    double end_time = MPI_Wtime();
    double elapsed_time = end_time - start_time;

    printf("Tiempo total de ejecución: %f segundos\n", elapsed_time);
    }
    MPI_Finalize();


    return 0;
}

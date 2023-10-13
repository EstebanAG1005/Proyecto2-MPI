#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

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

char search[] = "es una prueba de";
int tryKey(long key, unsigned char *ciph, int len, const char *search, unsigned char *decrypted_text){
    memcpy(decrypted_text, ciph, len);
    decrypted_text[len] = 0;
    decrypt(key, decrypted_text, len);
    
    // Elimina el relleno
    int padding_size = decrypted_text[len - 1];
    if (padding_size >= 1 && padding_size <= 8) {
        len -= padding_size;
        decrypted_text[len] = '\0';
    }
    
    return strstr((char *)decrypted_text, search) != NULL;
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <clave> <archivo_de_entrada>\n", argv[0]);
        return 1;
    }
    long theKey = strtol(argv[1], NULL, 10);
    const char *inputFile = argv[2]; 

    int processSize, id;
    long upper = (1L << 56);
    long myLower, myUpper;
    MPI_Status st;
    MPI_Request req;

    MPI_Comm comm = MPI_COMM_WORLD;
    double start, end;

    unsigned char *cipher;
    size_t cipherLen;

    readFileAndEncrypt(inputFile, theKey, &cipher, &cipherLen);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &processSize);
    MPI_Comm_rank(comm, &id);

    unsigned char *foundText = malloc(cipherLen + 1);
    int ready = 0;

    long rangePerNode = upper / processSize;
    myLower = rangePerNode * id;
    myUpper = rangePerNode * (id + 1) - 1;
    if (id == processSize - 1) {
        myUpper = upper;
    }

    if (id == 0) {
        printf("Procesando...\n");
        start = MPI_Wtime();
    }

    MPI_Irecv(foundText, cipherLen, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for (long i = myLower; i < myUpper; i += 16) { // Aplicación de saltos
        MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
        if (ready)
            break;

        for (int j = 0; j < 16; j++) {
            long keyToTry = i + j;
            if (tryKey(keyToTry, cipher, cipherLen, search, foundText)) {
                printf("Proceso %d encontró la clave: %li\n", id, keyToTry);
                end = MPI_Wtime();
                for (int node = 0; node < processSize; node++) {
                    MPI_Send(foundText, cipherLen, MPI_CHAR, node, 0, comm);
                }
                break;
            }
        }
    }

    if (id == 0) {
        MPI_Wait(&req, &st);
        printf("Tiempo para romper el cifrado DES: %f segundos\n", end - start);
    }

    free(cipher);
    free(foundText);
    MPI_Finalize();
    return 0;
}

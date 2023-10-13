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


void cleanAndPrintText(unsigned char *text, size_t len) {
    for(size_t i = 0; i < len; i++) {
        // Imprimir solo caracteres ASCII imprimibles o reemplazar con "."
        if(text[i] >= 32 && text[i] <= 126) {
            printf("%c", text[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}



int main(int argc, char *argv[]) {
    if(argc != 3) {
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

    for(key = rank; key <= upper_limit; key += size) {
        found = tryKey(key, cipher_text, cipher_len, argv[2], decrypted_text);

        if(found) {
            printf("Key found by process %d: %ld\n", rank, key);
            printf("Encrypted message: ");
            cleanAndPrintText(cipher_text, cipher_len);
            printf("Decrypted message: ");
            cleanAndPrintText(decrypted_text, cipher_len);
            break;
        }

    }

    free(cipher_text);
    free(decrypted_text);

    MPI_Finalize();
    return 0;
}


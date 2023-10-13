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

char search[] = " mensaje ";
unsigned char cipher[] = "Este es el mensaje que vamos a cifrar. Es multiplo de 8 bytes.    ";
long known_key = 0x0123456789ABCDEF;

int tryKey(long key, unsigned char *ciph, int len){
    unsigned char temp[len+1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr((char *)temp, search) != NULL;
}

int main(int argc, char *argv[]){
    int N, id;
    long upper = (1L <<56);
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    int flag;
    int ciphlen = strlen(cipher);
    MPI_Comm comm = MPI_COMM_WORLD;

    encrypt(known_key, cipher, ciphlen);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);
    int range_per_node = upper / N;
    mylower = range_per_node * id;
    myupper = range_per_node * (id+1) -1;

    if(id == N-1){
        myupper = upper;
    }

    long found = 0;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    for(int i = mylower; i<myupper && (found==0); ++i){
        if(tryKey(i, cipher, ciphlen)){
            found = i;
            for(int node=0; node<N; node++){
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    if(id==0){
        MPI_Wait(&req, &st);
        decrypt(found, cipher, ciphlen);
        printf("Found key: %li\n", found);
        printf("Decrypted message: %s\n", cipher);
    }

    MPI_Finalize();

    return 0;
}


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <stdint.h>


//Desencriptar
void decrypt(uint64_t key, char *ciph, int len) {
  DES_key_schedule ks;
  DES_cblock k;
  memcpy(k, &key, 8);
  DES_set_odd_parity(&k);
  DES_set_key_checked(&k, &ks);
  DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_DECRYPT);
}

//Encriptar
void encrypt(uint64_t key, char *ciph) {
  DES_key_schedule ks;
  DES_cblock k;
  memcpy(k, &key, 8);
  DES_set_odd_parity(&k);
  DES_set_key_checked(&k, &ks);
  DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_ENCRYPT);
}

// A la hora de leer y encriptar al parecer toma todo como uno en vez que cumpla con los 8 bytes, esto se asegura de eso
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


char *read_file_and_encrypt(const char *filename, long key) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_contents = (char *)malloc(file_size + 1);
    if (!file_contents) {
        perror("Error reservando memoria");
        exit(EXIT_FAILURE);
    }

    fread(file_contents, 1, file_size, file);
    fclose(file);

    file_contents[file_size] = '\0';

    // Agregar padding
    unsigned char *padded_text = addPadding((unsigned char*)file_contents, &file_size);
    free(file_contents);

    // Encriptando contenido del archivo con padding
    encrypt(key, (char *)padded_text);

    return (char *)padded_text;
}



//palabra clave a buscar en texto descifrado para determinar si se rompio el codigo
char search[] = "Esta";
int tryKey(uint64_t key, char *ciph, int len){
  char temp[len+1]; //+1 por el caracter terminal
  memcpy(temp, ciph, len);
  temp[len]=0; //caracter terminal
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

//key a probar: 42, 123456L, 18014398509481983L, 18014398509481984L
int main(int argc, char *argv[]){

  if (argc != 3) {
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

  char cipher[ciphlen+1];
  memcpy(cipher, eltexto, ciphlen);
  cipher[ciphlen]=0;

  //INIT MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  start_time = MPI_Wtime();

  long found = 0L;
  int ready = 0;

  //distribuir trabajo de forma naive
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) -1;
  if(id == N-1){
    //compensar residuo
    myupper = upper;
  }

  //non blocking receive, revisar en el for si alguien ya encontro
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  for(long i = mylower; i<myupper; ++i){
    MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
    if(ready)
      break;  //ya encontraron, salir

    if(tryKey(i, cipher, ciphlen)){
      found = i;
      printf("El proceso: %d encontro la llave\n", id);
      for(int node=0; node<N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, comm); //avisar a otros
      }
      break;
    }
  }

  //wait y luego imprimir el texto
  if(id==0){
    MPI_Wait(&req, &st);
    decrypt(found, cipher, ciphlen);
    printf("Se encontro la llave:\n");
    printf("Llave = %li\n\n", found);
    printf("Mensaje desencriptado:\n");
    printf("%s\n", cipher);
  }
  printf("Proceso %d terminando\n", id);


  end_time = MPI_Wtime();
  double elapsed_time = end_time - start_time;

  double max_elapsed_time;
  MPI_Reduce(&elapsed_time, &max_elapsed_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

  if (id == 0) {
    printf("Tiempo total de ejecucion: %f s\n", max_elapsed_time);
  }

  //FIN entorno MPI
  MPI_Finalize();
  free(eltexto);
}

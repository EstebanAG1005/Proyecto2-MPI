#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <stdint.h>


/**
 * La función descifra un texto cifrado dado usando el algoritmo DES con una llave especificada.
 * 
 * @param key La llave es un entero sin signo de 64 bits usado para cifrado y descifrado.
 * @param ciph El parámetro "ciph" es un puntero a una matriz de caracteres que representa el mensaje
 * cifrado. La longitud del mensaje cifrado está especificada por el parámetro "len".
 * @param len El parámetro "len" representa la longitud del texto cifrado (mensaje cifrado) en bytes.
 */
//Desencriptar
void decrypt(uint64_t key, char *ciph, int len) {
  DES_key_schedule ks;
  DES_cblock k;
  memcpy(k, &key, 8); // Convertir llave de 64 bits a bloque DES
  DES_set_odd_parity(&k); // Establecer paridad impar para la llave
  DES_set_key_checked(&k, &ks); // Preparar la llave para el cifrado
  DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_DECRYPT); // Descifrar en modo ECB
}

/**
 * La función cifra un mensaje dado utilizando el algoritmo de cifrado DES con una llave especificada.
 * 
 * @param key La llave es un entero sin signo de 64 bits usado para el cifrado.
 * @param ciph El parámetro "ciph" es un puntero a una matriz de caracteres que representa el texto plano o
 * el mensaje que desea cifrar.
 */
//Encriptar
void encrypt(uint64_t key, char *ciph) {
  DES_key_schedule ks;
  DES_cblock k;
  memcpy(k, &key, 8);
  DES_set_odd_parity(&k);
  DES_set_key_checked(&k, &ks);
  DES_ecb_encrypt((const_DES_cblock *)ciph, (DES_cblock *)ciph, &ks, DES_ENCRYPT);
}

/**
 * La función addPadding añade relleno a un texto para asegurar que su tamaño sea múltiplo de 8 bytes.
 * 
 * @param text El parámetro `text` es un puntero a un arreglo de caracteres sin signo, que representa
 * el texto original que necesita ser rellenado.
 * @param size El parámetro "size" es un puntero a una variable que contiene el tamaño del texto
 * original. Se pasa por referencia para que la función pueda actualizar su valor después de añadir relleno.
 * 
 * @return un puntero al texto rellenado.
 */
// A la hora de leer y encriptar al parecer toma todo como uno en vez que cumpla con los 8 bytes, esto se asegura de eso
// Función que agrega padding a un texto para que tenga un tamaño múltiplo de 8 bytes.
unsigned char* addPadding(unsigned char* text, size_t* size) {
    int padding_size = 8 - (*size % 8); // Calcular cuántos bytes de padding son necesarios
    unsigned char* padded_text = malloc(*size + padding_size + 1); // Reservar memoria para el texto con padding
    memcpy(padded_text, text, *size); // Copiar el texto original al nuevo buffer
    for (int i = 0; i < padding_size; i++) { 
        padded_text[*size + i] = padding_size; // Agregar bytes de padding
    }
    padded_text[*size + padding_size] = '\0'; // Terminador de cadena
    *size = *size + padding_size; // Actualizar el tamaño total
    return padded_text;
}

/**
 * La función lee un archivo, cifra su contenido, y devuelve el texto cifrado.
 * 
 * @param filename El parámetro filename es una cadena que representa el nombre del archivo a ser leído
 * y cifrado.
 * @param key El parámetro "key" es un entero largo que se utiliza para el cifrado. Se utiliza como un
 * parámetro para la función "encrypt", que no se muestra en el fragmento de código proporcionado. La función
 * "encrypt" probablemente toma la llave y el texto rellenado como entrada y ejecuta algún algoritmo de cifrado
 * sobre el.
 * 
 * @return un puntero al texto cifrado.
 */
// Leer un archivo, cifrar su contenido y devolver el texto cifrado.
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


/**
 * La función intenta descifrar un texto cifrado dado utilizando una llave y verifica si una palabra clave específica está
 * presente en el texto descifrado.
 * 
 * @param key La llave es un entero sin signo de 64 bits utilizado para el descifrado.
 * @param ciph El parámetro "ciph" es el texto cifrado que necesita ser descifrado.
 * @param len El parámetro "len" representa la longitud de la cadena de texto cifrado.
 * 
 * @return un valor booleano que indica si la palabra clave "Esta" se encuentra en el texto descifrado.
 */
//Palabra clave a buscar en texto descifrado para determinar si se rompio el codigo
char search[] = "Esta";
// Intentar descifrar el texto con una llave y buscar la palabra clave.
int tryKey(uint64_t key, char *ciph, int len){
  char temp[len+1]; //+1 por el caracter terminal
  memcpy(temp, ciph, len);
  temp[len]=0; //caracter terminal
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

/**
 * La función de arriba es un programa paralelo que intenta descifrar un texto cifrado dado usando un enfoque de
 * fuerza bruta al probar diferentes llaves en paralelo.
 * 
 * @param argc La variable `argc` representa el número de argumentos de línea de comandos pasados al
 * programa. En este caso, es el número total de argumentos incluyendo el nombre del programa en sí.
 * @param argv argv[0]: El nombre del archivo ejecutable
 */
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

  // Inicializar MPI
  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  start_time = MPI_Wtime();

  long found = 0L;
  int ready = 0;

  // Distribuir el trabajo entre los nodos
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) -1;
  if(id == N-1){
    myupper = upper;
  }
  // Recibir mensaje de manera no bloqueante
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);
  for(long i = mylower; i<myupper; ++i){
    MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
    if(ready)
      break;

    if(tryKey(i, cipher, ciphlen)){
      found = i;
      for(int node=0; node<N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, comm);
      }
      break;
    }
  }
  // Si es el proceso maestro, esperar mensaje y luego imprimir el texto descifrado
  if(id==0){
    MPI_Wait(&req, &st);
    decrypt(found, cipher, ciphlen);
    printf("Se encontro la llave: %li\n\n", found);
    printf("Mensaje desencriptado:\n");
    printf("%s\n", cipher);
  }
  printf("Proceso %d terminando\n", id);


  end_time = MPI_Wtime();
  double elapsed_time = end_time - start_time;
  // Reducir el tiempo máximo entre todos los nodos
  double max_elapsed_time;
  MPI_Reduce(&elapsed_time, &max_elapsed_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

  if (id == 0) {
    printf("Tiempo total de ejecucion: %f s\n", max_elapsed_time);
  }
  MPI_Finalize();
  free(eltexto);
}

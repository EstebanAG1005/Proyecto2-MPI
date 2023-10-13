#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

char *readFile(const char *filename)
{
  FILE *fp;
  char *buffer;
  long fileSize;

  fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    perror("Error opening file");
    return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  fileSize = ftell(fp);
  rewind(fp);

  buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL)
  {
    fclose(fp);
    perror("Error allocating memory");
    return NULL;
  }

  if (fread(buffer, fileSize, 1, fp) != 1)
  {
    fclose(fp);
    free(buffer);
    perror("Error reading file");
    return NULL;
  }

  buffer[fileSize] = '\0';
  fclose(fp);
  return buffer;
}

void decrypt(long key, char *ciph, int len)
{
  long k = 0;
  for (int i = 0; i < 8; ++i)
  {
    key <<= 1;
    k += (key & (0xFE << i * 8));
  }

  DES_cblock desKey;
  memcpy(desKey, &k, sizeof(k));
  DES_set_odd_parity(&desKey);

  DES_key_schedule keySchedule;
  DES_set_key_unchecked(&desKey, &keySchedule);

  for (size_t i = 0; i < len; i += 8)
  {
    DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &keySchedule, DES_DECRYPT);
  }

  size_t padLen = ciph[len - 1];
  if (padLen > 8)
  {
    return;
  }
  for (size_t i = len - padLen; i < len; i++)
  {
    if (ciph[i] != padLen)
    {
      return;
    }
  }

  ciph[len - padLen] = '\0';
}

void encryptText(long key, char *ciph, int len)
{
  long k = 0;
  for (int i = 0; i < 8; ++i)
  {
    key <<= 1;
    k += (key & (0xFE << i * 8));
  }

  DES_cblock desKey;
  memcpy(desKey, &k, sizeof(k));
  DES_set_odd_parity(&desKey);

  DES_key_schedule keySchedule;
  DES_set_key_unchecked(&desKey, &keySchedule);

  size_t padLen = 8 - len % 8;
  if (padLen == 0)
  {
    padLen = 8;
  }

  for (size_t i = len; i < len + padLen; i++)
  {
    ciph[i] = padLen;
  }

  for (size_t i = 0; i < len + padLen; i += 8)
  {
    DES_ecb_encrypt((DES_cblock *)(ciph + i), (DES_cblock *)(ciph + i), &keySchedule, DES_ENCRYPT);
  }
}

char search[] = "es una prueba de";

int tryKey(long key, char *ciph, int len)
{
  char temp[len + 1];
  strcpy(temp, ciph);
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

long theKey = 3L;

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    return 1;
  }
  theKey = strtol(argv[1], NULL, 10);

  int N, rank;
  long upper = (1L << 56);
  long realKey = 0;
  MPI_Status st;
  MPI_Request req;

  MPI_Comm comm = MPI_COMM_WORLD;
  double start, end;

  char *text = readFile("input.txt");
  if (text == NULL)
  {
    printf("Error reading \n");
    return 0;
  }

  int flag;
  int ciphLen = strlen((char *)text);

  char cipher[ciphLen + 1];
  memcpy(cipher, text, ciphLen);
  cipher[ciphLen] = 0;

  encryptText(theKey, cipher, ciphLen);
  printf("Cipher text: %s\n", cipher);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &N);

  start = MPI_Wtime();
  MPI_Irecv((void *)&realKey, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &req);
  int iterCount = 0;

  long keyToTry = rank;
  while (keyToTry < upper)
  {
    if (tryKey(keyToTry, (char *)cipher, ciphLen))
    {
      realKey = keyToTry;
      end = MPI_Wtime();
      int process = 0;
      while (process < N)
      {
        MPI_Send((void *)&realKey, 1, MPI_LONG, process, 0, MPI_COMM_WORLD);
        process++;
      }
      break;
    }
    if (++iterCount % 1000 == 0)
    {
      MPI_Test(&req, &flag, &st);
      if (flag)
        break;
    }
    keyToTry += N;
  }

  if (rank == 0)
  {
    MPI_Wait(&req, &st);
    decrypt(realKey, (char *)cipher, ciphLen);
    cipher[ciphLen + 1] = '\0';
    printf("%li %s \n", realKey, cipher);
    printf("Time %f", end - start);
  }

  MPI_Finalize();
}

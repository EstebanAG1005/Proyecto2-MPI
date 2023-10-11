#include <stdio.h>
#include <string.h>
#include <openssl/des.h>
#include <time.h>

// Función para verificar si el texto descifrado contiene la cadena objetivo
int isDecryptionSuccessful(const unsigned char *decryptedText, const char *targetString) {
    if (strstr((const char *)decryptedText, targetString) != NULL) {
        return 1; // Verdadero si 'targetString' está en 'decryptedText'
    }
    return 0;
}

int main() {
    // Reemplaza esto con tu texto cifrado y la cadena objetivo
    // Asegúrate de que 'encryptedText' esté en el formato correcto (es decir, un bloque de datos binarios de 64 bits)
    unsigned char encryptedText[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0}; // Ejemplo de texto cifrado
    const char *targetString = "target_string"; // la cadena que sabemos que está en el texto plano

    DES_cblock key;
    DES_key_schedule schedule;

    // Inicializa el tiempo de búsqueda
    clock_t begin = clock();

    for (long i = 0; i < 1000000; ++i) { // Este rango es solo para demostración
        // Genera una 'nueva' clave; en un escenario real, necesitarías una forma sistemática de generar todas las posibles claves
        memset(key, (int)i % 256, sizeof(DES_cblock)); // Esto solo está llenando la clave con un patrón de bytes

        // Establece la clave
        DES_set_key((const_DES_cblock *)&key, &schedule);

        // Buffer para el texto descifrado, ajusta el tamaño según sea necesario
        unsigned char decryptedText[64];

        // Descifra
        DES_cblock input, output; // Definir bloques de entrada y salida
        memcpy(input, encryptedText, sizeof(input)); // Copiar el texto cifrado al bloque de entrada
        DES_ecb_encrypt(&input, &output, &schedule, DES_DECRYPT);
        memcpy(decryptedText, output, sizeof(output)); // Copiar el bloque de salida al texto descifrado

        // Verifica si la desencriptación fue exitosa
        if (isDecryptionSuccessful(decryptedText, targetString)) {
            printf("Llave Encontrada: ");
            for (size_t j = 0; j < sizeof(DES_cblock); ++j) {
                printf("%02x", key[j]); // imprime la clave en hexadecimal
            }
            printf("\n");
            break;
        }
    }

    clock_t end = clock();
    double timeSpent = (double)(end - begin) / CLOCKS_PER_SEC;

    printf("Tiempo buscando: %f segundos\n", timeSpent);

    return 0;
}


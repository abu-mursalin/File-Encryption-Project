#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

// Function to derive AES key from password using SHA-256
void derive_key_from_password(const char *password, unsigned char *key) {
    SHA256((unsigned char *)password, strlen(password), key);
}

// AES encryption for files
void encrypt_file(const char *input_file, const char *output_file, const unsigned char *key) {
    FILE *in = fopen(input_file, "rb");
    FILE *out = fopen(output_file, "wb");
    if (!in || !out) {
        printf("File error!\n");
        exit(1);
    }

    AES_KEY enc_key;
    AES_set_encrypt_key(key, 256, &enc_key); // 256-bit AES key

    unsigned char in_block[16], out_block[16];
    int bytes_read;

    while ((bytes_read = fread(in_block, 1, 16, in)) > 0) {
        if (bytes_read < 16)
            memset(in_block + bytes_read, 16 - bytes_read, 16 - bytes_read); // PKCS#7 padding
        AES_encrypt(in_block, out_block, &enc_key);
        fwrite(out_block, 1, 16, out);
    }

    fclose(in);
    fclose(out);
    printf("✅ File encrypted successfully!\n");
}

// AES decryption for files
void decrypt_file(const char *input_file, const char *output_file, const unsigned char *key) {
    FILE *in = fopen(input_file, "rb");
    FILE *out = fopen(output_file, "wb");
    if (!in || !out) {
        printf("File error!\n");
        exit(1);
    }

    AES_KEY dec_key;
    AES_set_decrypt_key(key, 256, &dec_key);

    unsigned char in_block[16], out_block[16];
    int bytes_read;

    while ((bytes_read = fread(in_block, 1, 16, in)) > 0) {
        AES_decrypt(in_block, out_block, &dec_key);

        if (feof(in)) {
            int pad = out_block[15];
            if (pad > 0 && pad <= 16)
                fwrite(out_block, 1, 16 - pad, out);
            else
                fwrite(out_block, 1, 16, out);
        } else {
            fwrite(out_block, 1, 16, out);
        }
    }

    fclose(in);
    fclose(out);
    printf("✅ File decrypted successfully!\n");
}

int main() {
    int choice;
    char input_file[256], output_file[256], password[256];
    unsigned char key[32]; // 256-bit key

    printf("==== AES FILE ENCRYPTION (Password Protected) ====\n");
    printf("1. Encrypt file\n");
    printf("2. Decrypt file\n");
    printf("Enter choice: ");
    scanf("%d", &choice);

    printf("Enter input file name: ");
    scanf("%s", input_file);
    printf("Enter output file name: ");
    scanf("%s", output_file);
    printf("Enter password: ");
    scanf("%s", password);

    derive_key_from_password(password, key); // Create AES key from password

    if (choice == 1)
        encrypt_file(input_file, output_file, key);
    else if (choice == 2)
        decrypt_file(input_file, output_file, key);
    else
        printf("Invalid choice!\n");

    return 0;
}

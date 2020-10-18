//
// Created by tao on 2020/10/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

static unsigned char m_iv[16] = {0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58,
                                 0x56, 0x2e};


int main(int argc, const char *argv[]) {
    /*if (argc <= 1) {
        fprintf(stderr, "Usage: %s <data>\n", argv[0]);
        exit(EXIT_FAILURE);
    }*/
    char buffer[64];
    FILE *fp;
    const char *key, *data;
    int key_length, iv_length, data_length;
    key = "07wjrkc41typdvae";
    key_length = 16;
    iv_length = 16;
    fp = stdin;
    fread(buffer, 16, 1, fp);
    fclose(fp);
    data = buffer;
    data_length = 16;

    const EVP_CIPHER *cipher;
    int cipher_key_length, cipher_iv_length;
    cipher = EVP_aes_128_cbc();
    cipher_key_length = EVP_CIPHER_key_length(cipher);
    cipher_iv_length = EVP_CIPHER_iv_length(cipher);

    if (key_length != cipher_key_length) {
        fprintf(stderr, "Error: key length must be %d\n", cipher_key_length);
        exit(EXIT_FAILURE);
    }
    if (iv_length != cipher_iv_length) {
        fprintf(stderr, "Error: iv length must be %d\n", cipher_iv_length);
        exit(EXIT_FAILURE);
    }

    EVP_CIPHER_CTX *ctx;
    ctx = EVP_CIPHER_CTX_new();
    int i, cipher_length, final_length;
    unsigned char *ciphertext;

    EVP_CIPHER_CTX_init(ctx);
    EVP_CIPHER_CTX_set_padding(ctx, EVP_CIPH_NO_PADDING);
    EVP_EncryptInit_ex(ctx, cipher, NULL, (unsigned char *) key, (unsigned char *) m_iv);

    cipher_length = data_length + EVP_MAX_BLOCK_LENGTH;
    ciphertext = (unsigned char *) malloc(cipher_length);

    EVP_EncryptUpdate(ctx, ciphertext, &cipher_length, (unsigned char *) data, data_length);
    EVP_EncryptFinal_ex(ctx, ciphertext + cipher_length, &final_length);

    for (i = 0; i < cipher_length; i++)
        printf("%02X", ciphertext[i]);
    printf("\n");

    free(ciphertext);

    EVP_CIPHER_CTX_cleanup(ctx);

    return 0;
}

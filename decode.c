#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/*
 * Read and validate Decode args from argv
 * Usage: ./lsb_steg -d <stego.bmp> [output_file]
 * If no output file name is given, do_decoding() will pick a
 * default name once it knows the secret file's extension.
 */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    decInfo->stego_image_fname = argv[2];

    if (argv[3] != NULL)
    {
        decInfo->secret_fname = argv[3];
    }
    else
    {
        decInfo->secret_fname = NULL;
    }

    return e_success;
}

/*
 * Decode a single byte from the LSBs of 8 image bytes.
 * Inverse of encode_byte_to_lsb(): image_buffer[0] holds the
 * MSB of the original byte, image_buffer[7] holds the LSB.
 */
Status decode_byte_from_lsb(char *image_buffer, char *data)
{
    unsigned char byte = 0;

    for (int i = 0; i < 8; i++)
    {
        byte = (byte << 1) | (image_buffer[i] & 1);
    }

    *data = (char)byte;

    return e_success;
}

/*
 * Read `size` bytes worth of hidden data out of the stego image
 * (8 image bytes per hidden byte).
 */
Status decode_data_from_image(char *data, int size, FILE *fptr_stego_image)
{
    char image_buffer[8];

    for (int i = 0; i < size; i++)
    {
        if (fread(image_buffer, 8, 1, fptr_stego_image) != 1)
        {
            return e_failure;
        }

        decode_byte_from_lsb(image_buffer, &data[i]);
    }

    return e_success;
}

Status decode_magic_string(DecodeInfo *decInfo)
{
    printf("INFO: Decoding Magic String Signature\n");

    char magic[3];
    int magic_len = (int)strlen(MAGIC_STRING);

    if (decode_data_from_image(magic, magic_len, decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    magic[magic_len] = '\0';

    if (strcmp(magic, MAGIC_STRING) != 0)
    {
        return e_failure;
    }

    strcpy(decInfo->magic_string, magic);

    printf("INFO: Done\n");

    return e_success;
}

Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    printf("INFO: Decoding Secret File Extension\n");

    if (decode_data_from_image(decInfo->extn_secret_file, MAX_FILE_SUFFIX,
                               decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    decInfo->extn_secret_file[MAX_FILE_SUFFIX] = '\0';

    printf("INFO: Done\n");

    return e_success;
}

Status decode_secret_file_size(DecodeInfo *decInfo)
{
    printf("INFO: Decoding Secret File Size\n");

    char image_buffer[32];

    if (fread(image_buffer, 32, 1, decInfo->fptr_stego_image) != 1)
    {
        return e_failure;
    }

    unsigned long size = 0;

    for (int i = 0; i < 32; i++)
    {
        size = (size << 1) | (image_buffer[i] & 1);
    }

    decInfo->size_secret_file = (long)size;

    printf("INFO: Done\n");

    return e_success;
}

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    printf("INFO: Decoding Secret File Data\n");

    char ch;

    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        if (decode_data_from_image(&ch, 1, decInfo->fptr_stego_image) == e_failure)
        {
            return e_failure;
        }

        fwrite(&ch, 1, 1, decInfo->fptr_secret);
    }

    printf("INFO: Done\n");

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    printf("INFO: ## Decoding Procedure Started ##\n");

    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    printf("INFO: Opened stego image file\n");

    /* Skip the 54-byte BMP header; hidden data starts right after it */
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    if (decode_magic_string(decInfo) == e_failure)
    {
        printf("ERROR: Magic string mismatch - not a valid stego image, "
               "or no hidden data present\n");
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    printf("INFO: Magic string matched - valid stego image\n");

    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file extension\n");
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    printf("INFO: Decoded file extension: %s\n", decInfo->extn_secret_file);

    static char default_name[128];

    if (decInfo->secret_fname == NULL)
    {
        snprintf(default_name, sizeof(default_name), "decoded_output%s", decInfo->extn_secret_file);
        decInfo->secret_fname = default_name;
    }

    decInfo->fptr_secret = fopen(decInfo->secret_fname, "wb");

    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->secret_fname);
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    if (decode_secret_file_size(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file size\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_secret);
        return e_failure;
    }

    printf("INFO: Decoded secret file size: %ld bytes\n", decInfo->size_secret_file);

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file data\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_secret);
        return e_failure;
    }

    printf("INFO: ## Decoding Done Successfully ##\n");
    printf("INFO: Decoded data written to %s\n", decInfo->secret_fname);

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_secret);

    return e_success;
}

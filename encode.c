#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/*
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

        return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

        return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

        return e_failure;
    }

    // No failure return e_success
    return e_success;
}
OperationType check_operation_type(char *argv[])
{
    if (strcmp(argv[1], "-e") == 0)
    {
        return e_encode;
    }
    else if (strcmp(argv[1], "-d") == 0)
    {
        return e_decode;
    }
    else
    {
        return e_unsupported;
    }
}
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    // Store source image name
    encInfo->src_image_fname = argv[2];

    // Store secret file name
    encInfo->secret_fname = argv[3];

    // Check whether output file is given
    if (argv[4] != NULL)
    {
        encInfo->stego_image_fname = argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "steged_img.bmp";
        printf("INFO: Output File not mentioned. Creating steged_img.bmp as default\n");
    }

    return e_success;
}
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);

    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    /* Total bits we need to hide: magic string + fixed-width extension
     * + 32-bit size field + the secret data itself, each byte taking
     * 8 image bytes (1 bit per byte).
     */
    uint required_bytes =
        (uint)(strlen(MAGIC_STRING) + MAX_FILE_SUFFIX + sizeof(uint) + encInfo->size_secret_file);

    if (encInfo->image_capacity > required_bytes * 8)
    {
        return e_success;
    }

    return e_failure;
}
uint get_file_size(FILE *fptr)
{
    // Move to the end of the file
    fseek(fptr, 0, SEEK_END);

    // Get current position = file size
    uint size = ftell(fptr);

    // Move back to the beginning
    rewind(fptr);

    return size;
}
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];

    rewind(fptr_src_image);

    fread(header, 54, 1, fptr_src_image);

    fwrite(header, 54, 1, fptr_dest_image);

    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> (7 - i)) & 1);
    }

    return e_success;
}
Status do_encoding(EncodeInfo *encInfo)
{
    printf("INFO: ## Encoding Procedure Started ##\n");

    if (open_files(encInfo) == e_failure)
    {
        printf("ERROR: Failed to open files\n");
        return e_failure;
    }

    printf("INFO: Opened all required files\n");

    if (check_capacity(encInfo) == e_failure)
    {
        printf("ERROR: Insufficient image capacity to hide the secret file\n");
        return e_failure;
    }

    printf("INFO: Capacity OK\n");

    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("ERROR: Failed to copy BMP header\n");
        return e_failure;
    }

    printf("INFO: BMP Header copied\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode magic string\n");
        return e_failure;
    }

    char extn[MAX_FILE_SUFFIX + 1] = {0};

    if (get_file_extn(encInfo->secret_fname, extn) == e_failure)
    {
        printf("ERROR: Failed to determine secret file extension\n");
        return e_failure;
    }

    if (encode_secret_file_extn(extn, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file extension\n");
        return e_failure;
    }

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file size\n");
        return e_failure;
    }

    if (encode_secret_file_data(encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret file data\n");
        return e_failure;
    }

    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("ERROR: Failed to copy remaining image data\n");
        return e_failure;
    }

    printf("INFO: ## Encoding Done Successfully ##\n");

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}

/*
 * Extract the extension (including the dot) from a file name,
 * e.g. "secret.txt" -> ".txt". Always writes exactly MAX_FILE_SUFFIX
 * bytes into extn (null-padded) so it can be encoded/decoded at a
 * fixed width.
 */
Status get_file_extn(const char *fname, char *extn)
{
    const char *dot = strrchr(fname, '.');

    if (dot == NULL)
    {
        return e_failure;
    }

    memset(extn, 0, MAX_FILE_SUFFIX + 1);
    strncpy(extn, dot, MAX_FILE_SUFFIX);

    return e_success;
}
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    printf("INFO: Encoding Secret File Extension\n");

    encode_data_to_image((char *)file_extn, MAX_FILE_SUFFIX, encInfo->fptr_src_image,
                         encInfo->fptr_stego_image);

    printf("INFO: Done\n");

    return e_success;
}
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    printf("INFO: Encoding Secret File Size\n");

    char image_buffer[32];

    fread(image_buffer, 32, 1, encInfo->fptr_src_image);

    for (int i = 0; i < 32; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((file_size >> (31 - i)) & 1);
    }

    fwrite(image_buffer, 32, 1, encInfo->fptr_stego_image);

    printf("INFO: Done\n");

    return e_success;
}
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    printf("INFO: Encoding Secret File Data\n");

    char ch;

    while (fread(&ch, 1, 1, encInfo->fptr_secret) > 0)
    {
        encode_data_to_image(&ch, 1, encInfo->fptr_src_image, encInfo->fptr_stego_image);
    }

    printf("INFO: Done\n");

    return e_success;
}
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    printf("INFO: Encoding Magic String Signature\n");

    encode_data_to_image((char *)magic_string, strlen(magic_string), encInfo->fptr_src_image,
                         encInfo->fptr_stego_image);

    printf("INFO: Done\n");

    return e_success;
}
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;

    while (fread(&ch, 1, 1, fptr_src) > 0)
    {
        fwrite(&ch, 1, 1, fptr_dest);
    }

    return e_success;
}
Status encode_data_to_image(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char image_buffer[8];

    for (int i = 0; i < size; i++)
    {
        fread(image_buffer, 8, 1, fptr_src_image);

        encode_byte_to_lsb(data[i], image_buffer);

        fwrite(image_buffer, 8, 1, fptr_stego_image);
    }

    return e_success;
}
#ifndef DECODE_H
#define DECODE_H

#include "types.h"  // Contains user defined types
#include "encode.h" // Reuses MAX_FILE_SUFFIX and helper prototypes

/*
 * Structure to store information required for
 * decoding a secret file out of a stego Image.
 * Mirrors EncodeInfo on the encode side.
 */
typedef struct _DecodeInfo
{
    /* Stego Image info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

    /* Magic string read back from the image, for verification */
    char magic_string[3];

    /* Secret File Info */
    char *secret_fname; // Output file name (may be NULL -> default)
    FILE *fptr_secret;
    char extn_secret_file[MAX_FILE_SUFFIX + 1];
    long size_secret_file;

} DecodeInfo;

/* Read and validate Decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Decode Magic String */
Status decode_magic_string(DecodeInfo *decInfo);

/* Decode secret file extension */
Status decode_secret_file_extn(DecodeInfo *decInfo);

/* Decode secret file size */
Status decode_secret_file_size(DecodeInfo *decInfo);

/* Decode secret file data */
Status decode_secret_file_data(DecodeInfo *decInfo);

/* Decode function, which does the real decoding */
Status decode_data_from_image(char *data, int size, FILE *fptr_stego_image);

/* Decode a byte from the LSBs of 8 image bytes */
Status decode_byte_from_lsb(char *image_buffer, char *data);

#endif

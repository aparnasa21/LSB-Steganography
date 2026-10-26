#include <stdio.h>
#include "encode.h"
#include "decode.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage:\n");
        printf("./lsb_steg -e <input.bmp> <secret.txt> [output.bmp]\n");
        printf("./lsb_steg -d <stego.bmp> [output.txt]\n");
        return 1;
    }

    OperationType op_type = check_operation_type(argv);

    if (op_type == e_encode)
    {
        if (argc < 4)
        {
            printf("Usage: ./lsb_steg -e <input.bmp> <secret.txt> [output.bmp]\n");
            return 1;
        }

        EncodeInfo encInfo;

        if (read_and_validate_encode_args(argv, &encInfo) == e_success)
        {
            if (do_encoding(&encInfo) == e_success)
            {
                printf("Encoding completed successfully\n");
            }
            else
            {
                printf("Encoding failed\n");
                return 1;
            }
        }
    }
    else if (op_type == e_decode)
    {
        if (argc < 3)
        {
            printf("Usage: ./lsb_steg -d <stego.bmp> [output.txt]\n");
            return 1;
        }

        DecodeInfo decInfo;

        if (read_and_validate_decode_args(argv, &decInfo) == e_success)
        {
            if (do_decoding(&decInfo) == e_success)
            {
                printf("Decoding completed successfully\n");
            }
            else
            {
                printf("Decoding failed\n");
                return 1;
            }
        }
    }
    else
    {
        printf("Unsupported operation\n");
        return 1;
    }

    return 0;
}
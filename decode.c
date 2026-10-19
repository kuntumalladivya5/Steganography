#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "decode.h"
#include "types.h"
#include "common.h"


/* ---------------------------------------------------------
   Read and validate command line arguments
   --------------------------------------------------------- */

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] == NULL)
    {
        printf("ERROR: Stego image file not provided\n");
        return e_failure;
    }

    /* Check .bmp extension */
    char *dot = strrchr(argv[2], '.');

    if (dot == NULL || strcmp(dot, ".bmp") != 0)
    {
        printf("ERROR: Stego image must be a .bmp file\n");
        return e_failure;
    }

    decInfo->stego_image_fname = argv[2];

    /* Output file name */
    if (argv[3] != NULL)
    {
        decInfo->secret_fname = argv[3];
    }
    else
    {
        decInfo->secret_fname = "output";
    }

    return e_success;
}


/* ---------------------------------------------------------
   Open stego image
   --------------------------------------------------------- */

Status decode_open_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image =
        fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        printf("ERROR: Unable to open stego image\n");
        return e_failure;
    }

    printf("Stego image opened successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Check image capacity
   --------------------------------------------------------- */

Status decode_check_capacity(DecodeInfo *decInfo)
{
    decInfo->image_capacity =
        get_image_size_for_bmp(decInfo->fptr_stego_image);

    if (decInfo->image_capacity <= 0)
    {
        printf("ERROR: Invalid image capacity\n");
        return e_failure;
    }

    printf("Capacity check successful\n");

    return e_success;
}


/* ---------------------------------------------------------
   Decode one byte
   IMPORTANT:
   Encoder stores bits LSB first.
   --------------------------------------------------------- */

char decode_byte(char *image_buffer)
{
    char data = 0;

    for (int i = 0; i < 8; i++)
    {
        data |= (image_buffer[i] & 1) << i;
    }

    return data;
}


/* ---------------------------------------------------------
   Decode 32-bit integer
   IMPORTANT:
   Encoder stores size LSB first.
   --------------------------------------------------------- */

int decode_size(char *image_buffer)
{
    unsigned int size = 0;

    for (int i = 0; i < 32; i++)
    {
        size |= ((unsigned int)(image_buffer[i] & 1)) << i;
    }

    return (int)size;
}


/* ---------------------------------------------------------
   Decode magic string
   --------------------------------------------------------- */

Status decode_magic_string(const char *magic_string,
                           DecodeInfo *decInfo)
{
    char image_buffer[8];
    char decoded_char;

    for (int i = 0; magic_string[i] != '\0'; i++)
    {
        if (fread(image_buffer, 1, 8,
                  decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read image data\n");
            return e_failure;
        }

        decoded_char = decode_byte(image_buffer);

        if (decoded_char != magic_string[i])
        {
            printf("ERROR: Magic string mismatch\n");
            return e_failure;
        }
    }

    printf("Magic string decoded successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Decode secret file extension size
   --------------------------------------------------------- */

Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    char image_buffer[32];

    if (fread(image_buffer, 1, 32,
              decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unable to read extension size\n");
        return e_failure;
    }

    decInfo->secret_file_extn_size =
        decode_size(image_buffer);

    if (decInfo->secret_file_extn_size <= 0 ||
        decInfo->secret_file_extn_size >=
        (int)sizeof(decInfo->extn_secret_file))
    {
        printf("ERROR: Invalid extension size\n");
        return e_failure;
    }

    printf("Extension size decoded successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Decode secret file extension
   --------------------------------------------------------- */

Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    char image_buffer[8];

    for (int i = 0;
         i < decInfo->secret_file_extn_size;
         i++)
    {
        if (fread(image_buffer, 1, 8,
                  decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read file extension\n");
            return e_failure;
        }

        decInfo->extn_secret_file[i] =
            decode_byte(image_buffer);
    }

    decInfo->extn_secret_file[
        decInfo->secret_file_extn_size] = '\0';

    printf("File extension decoded successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Decode secret file size
   --------------------------------------------------------- */

Status decode_secret_file_size(DecodeInfo *decInfo)
{
    char image_buffer[32];

    if (fread(image_buffer, 1, 32,
              decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unable to read secret file size\n");
        return e_failure;
    }

    decInfo->size_secret_file =
        decode_size(image_buffer);

    if (decInfo->size_secret_file < 0)
    {
        printf("ERROR: Invalid secret file size\n");
        return e_failure;
    }

    printf("Secret file size decoded successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Create output file
   --------------------------------------------------------- */

Status decode_create_output_file(DecodeInfo *decInfo)
{
    char output_filename[100];

    snprintf(output_filename,
             sizeof(output_filename),
             "%s%s",
             decInfo->secret_fname,
             decInfo->extn_secret_file);

    decInfo->fptr_secret =
        fopen(output_filename, "wb");

    if (decInfo->fptr_secret == NULL)
    {
        printf("ERROR: Unable to create output file\n");
        return e_failure;
    }

    printf("Output file created successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Decode secret file data
   --------------------------------------------------------- */

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char image_buffer[8];
    char decoded_char;

    for (long i = 0;
         i < decInfo->size_secret_file;
         i++)
    {
        if (fread(image_buffer, 1, 8,
                  decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read secret data\n");
            return e_failure;
        }

        decoded_char = decode_byte(image_buffer);

        if (fwrite(&decoded_char, 1, 1,
                   decInfo->fptr_secret) != 1)
        {
            printf("ERROR: Unable to write secret data\n");
            return e_failure;
        }
    }

    printf("Secret file data decoded successfully\n");

    return e_success;
}


/* ---------------------------------------------------------
   Perform complete decoding
   --------------------------------------------------------- */

Status do_decoding(DecodeInfo *decInfo)
{
    /* Step 1: Open stego image */
    if (decode_open_files(decInfo) != e_success)
    {
        return e_failure;
    }


    /* Step 2: Check capacity */
    if (decode_check_capacity(decInfo) != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }


    /* Step 3: Move to pixel data
       BMP header = 54 bytes
    */

    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);


    /* Step 4: Decode magic string */

    if (decode_magic_string(MAGIC_STRING,
                            decInfo) != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }


    /* Step 5: Decode extension size */

    if (decode_secret_file_extn_size(decInfo)
        != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }


    /* Step 6: Decode extension */

    if (decode_secret_file_extn(decInfo)
        != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }


    /* Step 7: Decode secret file size */

    if (decode_secret_file_size(decInfo)
        != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }


    /* Step 8: Create output file */

    if (decode_create_output_file(decInfo)
        != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }


    /* Step 9: Decode secret data */

    if (decode_secret_file_data(decInfo)
        != e_success)
    {
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_secret);
        return e_failure;
    }


    /* Step 10: Close files */

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_secret);

    return e_success;
}
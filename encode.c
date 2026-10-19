#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"

#define MAGIC_STRING "#*"

/* Function Definitions */

/* Get image size */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;

    /* Seek to 18th byte */
    fseek(fptr_image, 18, SEEK_SET);

    /* Read width */
    fread(&width, sizeof(int), 1, fptr_image);

    /* Read height */
    fread(&height, sizeof(int), 1, fptr_image);

    /* Return image capacity */
    return width * height * 3;
}

/* Get file size */
uint get_file_size(FILE *fptr)
{
    uint size;

    fseek(fptr, 0, SEEK_END);

    size = ftell(fptr);

    rewind(fptr);

    return size;
}

/* Read and validate encoding arguments */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    /* Check source image */
    if (strstr(argv[2], ".bmp") == NULL)
    {
        printf("Source image must be a .bmp file\n");
        return e_failure;
    }

    encInfo->src_image_fname = argv[2];

    /* Check secret file */
    if (strstr(argv[3], ".txt") == NULL)
    {
        printf("Secret file must be a .txt file\n");
        return e_failure;
    }

    encInfo->secret_fname = argv[3];

    /* Set secret file extension */
    strcpy(encInfo->extn_secret_file, ".txt");

    /* Check output file */
    if (argv[4] != NULL)
    {
        if (strstr(argv[4],".bmp") == NULL)
        {
            printf("Output image must be a .bmp file\n");
            return e_failure;
        }

        encInfo->stego_image_fname = argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "stego.bmp";
    }

    return e_success;
}

/* Open required files */
Status open_files(EncodeInfo *encInfo)
{
    /* Source image */
    encInfo->fptr_src_image =fopen(encInfo->src_image_fname, "rb");

    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr,"ERROR: Unable to open file %s\n",encInfo->src_image_fname);
        return e_failure;
    }

    /* Secret file */
    encInfo->fptr_secret =fopen(encInfo->secret_fname, "rb");

    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr,"ERROR: Unable to open file %s\n", encInfo->secret_fname);
        return e_failure;
    }

    /* Stego image */
    encInfo->fptr_stego_image =
        fopen(encInfo->stego_image_fname, "wb");

    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr,
                "ERROR: Unable to open file %s\n",
                encInfo->stego_image_fname);
        return e_failure;
    }

    return e_success;
}

/* Check capacity */
Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity =
        get_image_size_for_bmp(encInfo->fptr_src_image);

    encInfo->size_secret_file =
        get_file_size(encInfo->fptr_secret);

    /*
     * Required capacity:
     *
     * 16 bits  -> magic string
     * 32 bits  -> extension size
     * extension
     * 32 bits  -> secret file size
     * secret file data
     */
    if (encInfo->image_capacity >
        16 +
        32 +
        (strlen(encInfo->extn_secret_file) * 8) +
        32 +
        (encInfo->size_secret_file * 8))
    {
        return e_success;
    }

    return e_failure;
}

/* Copy BMP header */
Status copy_bmp_header(FILE *fptr_src_image,
                       FILE *fptr_dest_image)
{
    char header[54];

    rewind(fptr_src_image);

    if (fread(header, 1, 54, fptr_src_image) != 54)
    {
        return e_failure;
    }

    if (fwrite(header, 1, 54, fptr_dest_image) != 54)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode magic string */
Status encode_magic_string(const char *magic_string,
                           EncodeInfo *encInfo)
{
    char buffer[8];

    for (int i = 0; i < strlen(magic_string); i++)
    {
        /* Read 8 bytes */
        if (fread(buffer, 1, 8,
                  encInfo->fptr_src_image) != 8)
        {
            return e_failure;
        }

        /* Encode character */
        if (encode_byte_to_lsb(magic_string[i],
                               buffer) != e_success)
        {
            return e_failure;
        }

        /* Write modified bytes */
        if (fwrite(buffer, 1, 8,
                   encInfo->fptr_stego_image) != 8)
        {
            return e_failure;
        }
    }

    return e_success;
}

/* Encode secret file extension size */
Status encode_secret_file_extn_size(int size,
                                    EncodeInfo *encInfo)
{
    char buffer[32];

    /* Read 32 bytes */
    if (fread(buffer, 1, 32,
              encInfo->fptr_src_image) != 32)
    {
        return e_failure;
    }

    /* Encode size */
    if (encode_size_to_lsb(size, buffer) != e_success)
    {
        return e_failure;
    }

    /* Write modified bytes */
    if (fwrite(buffer, 1, 32,
               encInfo->fptr_stego_image) != 32)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode secret file extension */
Status encode_secret_file_extn(const char *file_extn,
                               EncodeInfo *encInfo)
{
    char buffer[8];

    for (int i = 0; i < strlen(file_extn); i++)
    {
        /* Read 8 bytes */
        if (fread(buffer, 1, 8,
                  encInfo->fptr_src_image) != 8)
        {
            return e_failure;
        }

        /* Encode one character */
        if (encode_byte_to_lsb(file_extn[i],
                               buffer) != e_success)
        {
            return e_failure;
        }

        /* Write modified bytes */
        if (fwrite(buffer, 1, 8,
                   encInfo->fptr_stego_image) != 8)
        {
            return e_failure;
        }
    }

    return e_success;
}

/* Encode secret file size */
Status encode_secret_file_size(long file_size,
                               EncodeInfo *encInfo)
{
    char buffer[32];

    /* Read 32 bytes */
    if (fread(buffer, 1, 32,
              encInfo->fptr_src_image) != 32)
    {
        return e_failure;
    }

    /* Encode file size */
    if (encode_size_to_lsb((int)file_size,
                           buffer) != e_success)
    {
        return e_failure;
    }

    /* Write modified bytes */
    if (fwrite(buffer, 1, 32,
               encInfo->fptr_stego_image) != 32)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode secret file data */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char buffer[8];
    char ch;

    rewind(encInfo->fptr_secret);

    /*
     * Read secret file one character at a time
     */
    while (fread(&ch, 1, 1,
                 encInfo->fptr_secret) == 1)
    {
        /* Read 8 bytes from source image */
        if (fread(buffer, 1, 8,
                  encInfo->fptr_src_image) != 8)
        {
            return e_failure;
        }

        /* Encode character */
        if (encode_byte_to_lsb(ch, buffer) != e_success)
        {
            return e_failure;
        }

        /* Write encoded bytes */
        if (fwrite(buffer, 1, 8,
                   encInfo->fptr_stego_image) != 8)
        {
            return e_failure;
        }
    }

    return e_success;
}

/* Copy remaining image data */
Status copy_remaining_img_data(FILE *fptr_src,
                               FILE *fptr_dest)
{
    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read =
            fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, fptr_dest)
            != bytes_read)
        {
            return e_failure;
        }
    }

    if (ferror(fptr_src))
    {
        return e_failure;
    }

    return e_success;
}

/* Encode one byte into LSB */
Status encode_byte_to_lsb(char data,
                          char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] =
            (image_buffer[i] & 0xFE) |
            ((data >> i) & 1);
    }

    return e_success;
}

/* Encode integer into LSB */
Status encode_size_to_lsb(int size,
                          char *imageBuffer)
{
    for (int i = 0; i < 32; i++)
    {
        imageBuffer[i] =
            (imageBuffer[i] & 0xFE) |
            ((size >> i) & 1);
    }

    return e_success;
}

/* Main encoding function */
Status do_encoding(EncodeInfo *encInfo)
{
    /* Step 1: Open files */
    if (open_files(encInfo) != e_success)
    {
        printf("ERROR: Unable to open files\n");
        return e_failure;
    }

    printf("Opened source and destination files successfully\n");

    /* Step 2: Check capacity */
    if (check_capacity(encInfo) != e_success)
    {
        printf("ERROR: Insufficient image capacity\n");
        return e_failure;
    }

    printf("Image has sufficient capacity\n");

    /* Step 3: Copy BMP header */
    if (copy_bmp_header(encInfo->fptr_src_image,
                        encInfo->fptr_stego_image) != e_success)
    {
        printf("ERROR: Failed to copy BMP header\n");
        return e_failure;
    }

    printf("BMP header copied successfully\n");

    /* Step 4: Encode magic string */
    if (encode_magic_string(MAGIC_STRING,
                            encInfo) != e_success)
    {
        printf("ERROR: Failed to encode magic string\n");
        return e_failure;
    }

    printf("Magic string encoded successfully\n");

    /* Step 5: Encode extension size */
    if (encode_secret_file_extn_size(
            strlen(encInfo->extn_secret_file),
            encInfo) != e_success)
    {
        printf("ERROR: Failed to encode extension size\n");
        return e_failure;
    }

    printf("Extension size encoded successfully\n");

    /* Step 6: Encode extension */
    if (encode_secret_file_extn(
            encInfo->extn_secret_file,
            encInfo) != e_success)
    {
        printf("ERROR: Failed to encode file extension\n");
        return e_failure;
    }

    printf("File extension encoded successfully\n");

    /* Step 7: Encode secret file size */
    if (encode_secret_file_size(
            encInfo->size_secret_file,
            encInfo) != e_success)
    {
        printf("ERROR: Failed to encode secret file size\n");
        return e_failure;
    }

    printf("Secret file size encoded successfully\n");

    /* Step 8: Encode secret data */
    if (encode_secret_file_data(encInfo) != e_success)
    {
        printf("ERROR: Failed to encode secret file data\n");
        return e_failure;
    }

    printf("Secret file data encoded successfully\n");

    /* Step 9: Copy remaining image data */
    if (copy_remaining_img_data(
            encInfo->fptr_src_image,
            encInfo->fptr_stego_image) != e_success)
    {
        printf("ERROR: Failed to copy remaining image data\n");
        return e_failure;
    }

    printf("Remaining image data copied successfully\n");

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}


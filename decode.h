#define DECODE_H

#include <stdio.h>
#include "types.h"

#define MAGIC_STRING "#*"

typedef struct _DecodeInfo
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char *secret_fname;
    FILE *fptr_secret;

    long image_capacity;

    char extn_secret_file[20];
    int secret_file_extn_size;

    long size_secret_file;

} DecodeInfo;


/* Function declarations */

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

Status decode_open_files(DecodeInfo *decInfo);

Status decode_check_capacity(DecodeInfo *decInfo);

char decode_byte(char *image_buffer);

int decode_size(char *image_buffer);

Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo);

Status decode_secret_file_extn_size(DecodeInfo *decInfo);

Status decode_secret_file_extn(DecodeInfo *decInfo);

Status decode_secret_file_size(DecodeInfo *decInfo);

Status decode_secret_file_data(DecodeInfo *decInfo);

uint get_image_size_for_bmp(FILE *fptr_image);

Status do_decoding(DecodeInfo *decInfo);


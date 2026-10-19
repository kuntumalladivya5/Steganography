#include <stdio.h>
#include <string.h>
#include "encode.h" 
#include "decode.h" 
#include "types.h" 

OperationType check_operation_type(char *symbol); 
int main(int argc, char *argv[]) 
{
     if (argc < 3) 
     { 
        printf("ERROR: Insufficient arguments\n"); 
        return e_failure; 
    }
     OperationType operation = check_operation_type(argv[1]);
      if (operation == e_encode) 
      { 
        EncodeInfo encInfo; 
        if (read_and_validate_encode_args(argv, &encInfo) == e_success) 
        { 
            if (do_encoding(&encInfo) == e_success) 
            { 
                printf("Encoding completed successfully\n"); 
            } 
            else 
            { 
                printf("ERROR: Encoding failed\n"); 
                return e_failure; 
            } 
        } 
        else 
        { 
            printf("ERROR: Invalid encoding arguments\n"); 
            return e_failure;
        } 
    }
     else if (operation == e_decode) { DecodeInfo decInfo; 
        if (read_and_validate_decode_args(argv, &decInfo) == e_success) { 
            if (do_decoding(&decInfo) == e_success)
             {
                 printf("Decoding completed successfully\n"); 
            } 
            else 
            {
                 printf("ERROR: Decoding failed\n"); 
                 return e_failure; 
                }
             } 
             else 
             {
                 printf("ERROR: Invalid decoding arguments\n"); 
                 return e_failure; 
                }
             }
              else 
              { 
                printf("ERROR: Unsupported operation\n"); 
                return e_failure;
             }
              return e_success;
             }
              OperationType check_operation_type(char *symbol) 
              {
                 if (strcmp(symbol, "-e") == 0)
                  {
                     return e_encode; 
                    }
                     else if (strcmp(symbol, "-d") == 0) 
                     {
                         return e_decode; 
                        }
                         else 
                         {
                             return e_unsupported;
                            }
                         }
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gsm_ultilities.h"

uint32_t gsm_utilities_get_number_from_string(uint16_t begin_index, char *buffer)
{
    // assert(buffer);

    uint32_t value = 0;
    uint16_t tmp = begin_index;
    uint32_t len = strlen(buffer);
    while (buffer[tmp] && tmp < len)
    {
        if (buffer[tmp] >= '0' && buffer[tmp] <= '9')
        {
            value *= 10;
            value += buffer[tmp] - 48;
        }
        else
        {
            break;
        }
        tmp++;
    }

    return value;
}

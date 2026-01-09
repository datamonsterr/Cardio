#include <stdio.h>
int compare_raw_bytes(char* b1, char* b2, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (b1[i] != b2[i])
        {
            return 0;
        }
    }

    return 1;
}
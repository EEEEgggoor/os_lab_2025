#include "revert_string.h"

void RevertString(char *str, int len)
{
	for(int i = 0; i < len/2; i++){
        char temp;
        temp = str[i];
        str[i] = str[len -1 - i];
        str[len - i - 1] = temp;
    }
}

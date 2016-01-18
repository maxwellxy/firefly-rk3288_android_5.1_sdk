#include <stdio.h>
#include <stdlib.h>

#include <codec_test.h>
#include "script.h"

int main(int argc, char **argv)
{
    printf ("\r\nBEGIN CODEC TEST\r\n");
    if(strcmp(argv[0], "case2") == 0) {
    	rec_play_test_2(NULL);
	} else {
		rec_play_test_1(NULL);
	}
    printf ("\r\nEND CODEC TEST\r\n");
    return 0;
}

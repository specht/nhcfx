#include <stdio.h>
#include <math.h>
#include <stdlib.h>

void main(int argc, char** argv)
{
    unsigned char r = atoi(argv[1]);
    unsigned char g = atoi(argv[2]);
    unsigned char b = atoi(argv[3]);
    unsigned char a = 0;
    while (!feof(stdin))
    {
        fread(&a, 1, 1, stdin);
        fwrite(&r, 1, 1, stdout);
        fwrite(&g, 1, 1, stdout);
        fwrite(&b, 1, 1, stdout);
        fwrite(&a, 1, 1, stdout);
    }
}

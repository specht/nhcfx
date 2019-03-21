#include <stdio.h>
#include <math.h>
#include <stdlib.h>

void main(int argc, char** argv)
{
    unsigned char rgba[4];
    for (int i = 0; i < 3; i++)
        rgba[i] = atoi(argv[i + 1]);
    while (!feof(stdin))
    {
        fread(rgba + 3, 1, 1, stdin);
        fwrite(rgba, 4, 1, stdout);
    }
}

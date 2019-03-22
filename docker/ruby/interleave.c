#include <stdio.h>
#include <math.h>
#include <stdlib.h>

void main(int argc, char** argv)
{
    unsigned char rgba[4];
    unsigned char opacity = 0xff;
    for (int i = 0; i < 3; i++)
        rgba[i] = atoi(argv[i + 1]);
    if (argc > 4)
        opacity = atoi(argv[4]);
    while (!feof(stdin))
    {
        fread(rgba + 3, 1, 1, stdin);
        rgba[3] = (((int)rgba[3]) * opacity) / 255;
        fwrite(rgba, 4, 1, stdout);
    }
}

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "/src/3rd_party/tinyexpr/tinyexpr.h"

const int width = #{WIDTH};
const int height = #{HEIGHT};
const int width2 = #{WIDTH} / 2;
const int height2 = #{WIDTH} / 2;
float xl = #{XL}, yt = #{YT}, dx = #{DX}, dy = #{DY};
int aa_level = #{AA_LEVEL};

double x, y, r, phi;
te_variable vars[] = {{"x", &x}, {"y", &y}, {"r", &r}, {"phi", &phi}};
int err;
te_expr *n;
    
unsigned char f(float _x, float _y)
{
    x = _x;
    y = _y;
//(R)   r = sqrt(x * x + y * y);
//(PHI) phi = atan2(y, x);
    return te_eval(n) < 0;
}

float subdivide(float sx, float sy, int level, int max_level)
{
    float step = 1.0 / (1 << level);
    float step2 = step * 0.5;
    float x = xl + (sx + step) * dx;
    float y = yt + (sy + step) * dy;
    return f(x, y) ? 1.0 : 0.0;
//     // adaptive super sampling
//     if (count != 0 && count != 4 && level < max_level)
//     {
//         return (subdivide(sx, sy, level + 1, max_level) +
//                 subdivide(sx + step2, sy, level + 1, max_level) +
//                 subdivide(sx, sy + step2, level + 1, max_level) + 
//                 subdivide(sx + step2, sy + step2, level + 1, max_level)) * 0.25;
//     }
//     else
//         return count ? 1.0 : 0.0;
}

void main(int argc, char** argv)
{
    n = te_compile("#{FUNCTION}", vars, 4, &err);
    if (err)
    {
        fprintf(stderr, "te_compile_error:%d\n", err);
        exit(1);
    }
    unsigned char* scanlines = malloc(sizeof(unsigned char) * (width + 1) * 2);
    unsigned char* line0 = scanlines;
    unsigned char* line1 = scanlines + width + 1;
    int sx0 = round(-xl / dx);
    int sy0 = round(-yt / dy);
    unsigned int offset = 0;

//     FILE *f = fopen(argv[1], "w");
//     fprintf(f, "%d %d", label_x, label_y);
//     fclose(f);
    
    // dilate pixels and render graph
    int old_percent = -1;
    for (int y = 0; y < height; y++)
    {
        int percent = y * 100 / height;
        if (percent != old_percent)
        {
            old_percent = percent;
            fprintf(stderr, "\r%d", percent);
        }
        for (int x = 0; x < width; x++)
        {
            unsigned char color = 0;
            color = round(subdivide(x, y, 0, aa_level) * 255.0);
            fwrite(&color, 1, 1, stdout);
        }
    }
    fprintf(stderr, "\r%d\n", 100);
    
    free(scanlines);
    
    te_free(n);
}

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "/src/3rd_party/tinyexpr/tinyexpr.h"

const int width = #{WIDTH};
const int height = #{HEIGHT};
const int width2 = #{WIDTH} / 2;
const int height2 = #{WIDTH} / 2;
float xl = #{XL}, yt = #{YT}, dx = #{DX}, dy = #{DY};
float *pd;
float lw = #{LINE_WIDTH}; // line width in pixels
int aa_level = #{AA_LEVEL};
int np = #{PEN_POINTS}; // number of points of pen polygon

double x, y, r, phi;
te_variable vars[] = {{"x", &x}, {"y", &y}, {"r", &r}, {"phi", &phi}};
int err;
te_expr *n;
    
float f(float _x, float _y)
{
    x = _x;
    y = _y;
//(R)   r = sqrt(x * x + y * y);
//(PHI) phi = atan2(y, x);
    return te_eval(n);
//     return (#{FUNCTION});
}

float subdivide(float sx, float sy, int level, int max_level)
{
    float step = 1.0 / (1 << level);
    float step2 = step * 0.5;
    int count = 0;
    for (int tx = 0; tx < 2; tx++)
    {
        for (int ty = 0; ty < 2; ty++)
        {
            float* pdx = pd + 0;
            float* pdy = pd + 1;
            float x = xl + (sx + tx * step) * dx;
            float y = yt + (sy + ty * step) * dy;
            int sub_count = 0;
            for (int i = 0; i < np; i++) {
                if (f(x + (*pdx), y + (*pdy)) < 0)
                    sub_count++;
                pdx += 2;
                pdy += 2;
            }
            if (sub_count > 0 && sub_count < np)
                count += 1;
        }
    }
    // adaptive super sampling
    if (count != 0 && count != 4 && level < max_level)
    {
        return (subdivide(sx, sy, level + 1, max_level) +
                subdivide(sx + step2, sy, level + 1, max_level) +
                subdivide(sx, sy + step2, level + 1, max_level) + 
                subdivide(sx + step2, sy + step2, level + 1, max_level)) * 0.25;
    }
    else
        return count ? 1.0 : 0.0;
}

void main(int argc, char** argv)
{
    n = te_compile("#{FUNCTION}", vars, 4, &err);
    unsigned char* buffer;
    pd = malloc(sizeof(float) * 2 * np);
    float ssx = dx * 0.001;
    float ssy = dy * 0.001;
    float* scanlines = malloc(sizeof(float) * (width + 1) * 2);
    float* line0 = scanlines;
    float* line1 = scanlines + width + 1;
    int sx0 = round(-xl / dx);
    int sy0 = round(-yt / dy);
    buffer = malloc(width * height);
    unsigned int offset = 0;
    
    for (int i = 0; i < np; i++)
    {
        pd[i * 2 + 0] = cos((float)i * 2 * M_PI / np) * lw * 0.5 * dx;
        pd[i * 2 + 1] = sin((float)i * 2 * M_PI / np) * lw * 0.5 * dy;
    }
    
    // find seed pixels
    float y = yt;
    int label_x = -1;
    int label_y = -1;
    long label_dist = 0;
    for (int sy = 0; sy < height + 1; sy++)
    {
        float* slp = line0;
        float x = xl;
        for (int sx = 0; sx < width + 1; sx++)
        {
            float d = f(x, y);
            *(slp++) = d;
            if (sx > 0 && sy > 0)
            {
                unsigned char hit = 0;
                float p0, p1, p2;
                p0 = line1[sx - 1];
                p2 = line0[sx];
                if (p0 * p2 < 0)
                {
                    p1 = f(x - ssx, y - ssy);
                    if (p0 * p1 < 0 && fabs(p0 - p1) < fabs(p0 - p2))
                        hit = 1;
                    if (!hit)
                    {
                        p1 = f(x - dx + ssx, y - dy + ssy);
                        if (p1 * p2 < 0 && fabs(p1 - p2) < fabs(p0 - p2))
                            hit = 1;
                    }
                }
                p0 = line1[sx];
                p2 = line0[sx - 1];
                if (!hit && p0 * p2 < 0)
                {
                    p1 = f(x - dx + ssx, y - ssy);
                    if (p0 * p1 < 0 && fabs(p0 - p1) < fabs(p0 - p2))
                        hit = 1;
                    if (!hit)
                    {
                        p1 = f(x - ssx, y - dy + ssy);
                        if (p1 * p2 < 0 && fabs(p1 - p2) < fabs(p0 - p2))
                            hit = 1;
                    }
                }
                buffer[offset++] = hit;
                if (hit) 
                {
                    if (sx == 1 || sy == 1 || sx == width || sy == height)
                    {
                        long dist = abs((sy - sy0) * (sx - sx0));
                        if (dist > label_dist)
                        {
                            label_x = sx - 1;
                            label_y = sy - 1;
                            label_dist = dist;
                        }
                    }
                }
            }
            x += dx;
        }
        y += dy;
        float* temp = line0;
        line0 = line1;
        line1 = temp;
    }

    FILE *f = fopen(argv[1], "w");
    fprintf(f, "%d %d", label_x, label_y);
    fclose(f);
    
    // dilate pixels and render graph
    int w = ceil(lw);
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
            unsigned char found_pixel = 0;
            for (int dy = -w; !found_pixel && dy <= w; dy++)
            {
                for (int dx = -w; !found_pixel && dx <= w; dx++)
                {
                    int tx = x + dx;
                    int ty = y + dy;
                    if (tx >= 0 && tx < width && ty >= 0 && ty < height)
                    {
                        if (buffer[ty * width + tx])
                            found_pixel = 1;
                    }
                }
            }
            // we have found a dilated pixel near the graph, 
            // now render the graph at this pixel
            unsigned char color = 0;
            if (found_pixel) 
                color = round(subdivide(x, y, 0, aa_level) * 255.0);
            fwrite(&color, 1, 1, stdout);
        }
    }
    fprintf(stderr, "\r%d\n", 100);
    
    free(buffer);
    free(scanlines);
    free(pd);
    
    te_free(n);
}

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>

using namespace std;

// gcc -O1 -o /app_bin/nhcfx -I/src/ext/exprtk/ /src/nhcfx.cpp -lstdc++ -lm && time /app_bin/nhcfx 0 "sin(x)-y" 236 236 -4 4 0.03389830508474576 -0.03389830508474576 2 2.362204724409449 8 /tmp/coords.txt | convert -size 236x236 -depth 8 gray:- /cache/out.png

// #define DUMMY

#ifndef DUMMY
#include "exprtk.hpp"
#endif

double x, y;
double* r_ = 0;
double* phi_ = 0;
#ifndef DUMMY
exprtk::symbol_table<double> symbol_table;
exprtk::expression<double> expression;
exprtk::parser<double> parser;
#endif

// Arguments:
//  1. type (int: 0 = eq / 1 = lt)
//  2. function (string)
//  3. width (int)
//  4. height (int)
//  5. x left (double)
//  6. y top (double)
//  7. delta x (double)
//  8. delta y (double)
//  9. antialiasing level (int)
// 10. line width (double, only if type == eq)
// 11. pen points (int, only if type == eq)
// 12. output text file for label coordinates

int type;
const char* fx;
int width, height, width2, height2;
double xl, yt, dx, dy;
int aa_level;
double lw;
int np;
double *pd;

inline double ln(double x)
{
   return log(x);
}

double f(double _x, double _y)
{
    x = _x;
    y = _y;
    if (r_)
        *r_ = sqrt(x * x + y * y);
    if (phi_)
        *phi_ = atan2(y, x);
#ifndef DUMMY
    return expression.value();
#else
    return r - (2.5 + (fmod((phi+M_PI+0.2) , (M_PI/8))) * 2);
//     return r - 3;
#endif
}

double subdivide(double sx, double sy, int level, int max_level)
{
    double step = 1.0 / (1 << level);
    double step2 = step * 0.5;
    int count = 0;
    for (int tx = 0; tx < 2; tx++)
    {
        for (int ty = 0; ty < 2; ty++)
        {
            double* pdx = pd + 0;
            double* pdy = pd + 1;
            double x = xl + (sx + tx * step) * dx;
            double y = yt + (sy + ty * step) * dy;
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

int main(int argc, char** argv)
{
    type = atoi(argv[1]);
    fx = argv[2];
    width = atoi(argv[3]);
    height = atoi(argv[4]);
    width2 = width / 2;
    height2 = height / 2;
    xl = atof(argv[5]);
    yt = atof(argv[6]);
    dx = atof(argv[7]);
    dy = atof(argv[8]);
    aa_level = atoi(argv[9]);
    lw = atof(argv[10]);
    np = atoi(argv[11]);

#ifndef DUMMY
    symbol_table.add_variable("x", x);
    symbol_table.add_variable("y", y);
    symbol_table.add_constant("e", 2.718281828459045235360287);
    symbol_table.add_function("ln", ln);
    symbol_table.add_constants();
    exprtk::symbol_table<double> unknown_symbol_table;
    expression.register_symbol_table(unknown_symbol_table);
    expression.register_symbol_table(symbol_table);
    parser.enable_unknown_symbol_resolver();
    parser.compile(fx, expression);
    std::vector<std::string> variable_list;
    unknown_symbol_table.get_variable_list(variable_list);
    for (auto& var_name : variable_list)
    {
        if (var_name.compare("r") == 0)
            r_ = &unknown_symbol_table.variable_ref(var_name);
        else if (var_name.compare("phi") == 0)
            phi_ = &unknown_symbol_table.variable_ref(var_name);
        else
        {
            cerr << "unknown variable: " << var_name << "\n";
            exit(1);
        }
    }
#endif

    unsigned char* buffer;
    pd = (double*)malloc(sizeof(double) * 2 * np);
    double ssx = dx * 0.001;
    double ssy = dy * 0.001;
    double* scanlines = (double*)malloc(sizeof(double) * (width + 1) * 2);
    double* line0 = scanlines;
    double* line1 = scanlines + width + 1;
    int sx0 = round(-xl / dx);
    int sy0 = round(-yt / dy);
    buffer = (unsigned char*)malloc(width * height);
    unsigned int offset = 0;
    
    for (int i = 0; i < np; i++)
    {
        pd[i * 2 + 0] = cos((double)i * 2 * M_PI / np) * lw * 0.5 * dx;
        pd[i * 2 + 1] = sin((double)i * 2 * M_PI / np) * lw * 0.5 * dy;
    }
    
    // find seed pixels
    double y = yt;
    int label_x = -1;
    int label_y = -1;
    long label_dist = 0;
    for (int sy = 0; sy < height + 1; sy++)
    {
        double* slp = line0;
        double x = xl;
        for (int sx = 0; sx < width + 1; sx++)
        {
            double d = f(x, y);
            *(slp++) = d;
            if (sx > 0 && sy > 0)
            {
                unsigned char hit = 0;
                double p0, p1, p2;
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
        double* temp = line0;
        line0 = line1;
        line1 = temp;
    }

    FILE *fl = fopen(argv[12], "w");
    fprintf(fl, "%d %d", label_x, label_y);
    fclose(fl);
    
    // dilate pixels and render graph
    int w = ceil(lw / 2);
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
    
    return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>

using namespace std;

// gcc -O1 -o /app_bin/nhcfx -I/src/ext/exprtk/ /src/nhcfx.cpp -lstdc++ -lm && time /app_bin/nhcfx 0 "0.02/x - (y)" 590 590 -1 1 0.003389830508474576 -0.003389830508474576 2 29.52755905511811 8 /tmp/coords.txt | convert -size 590x590 -depth 8 gray:- /cache/out.png

#define DUMMY

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
// 13. etc. pairs of variable names and values

int type;
const char* fx;
int width, height, width2, height2;
double xl, yt, dx, dy;
int aa_level;
double lw;
int np;
double *pd; // pen points
double *pdm; // midpoint between point n and n+1
double *fv; // values at pen points
bool *fb; // sign at pen points (true == negative)
int (*get_color)(double, double);

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
    return 0.02/x-y;
//     return y - sin(x);
//     return r - 3;
#endif
}

int get_color_area(double x, double y)
{
    return static_cast<int>(f(x, y));
}

int get_color_line(double x, double y)
{
//     double v = f(x, y);
//     if (v < 0)
//     {
//         return 0;
//     }
//     else
//     {
//         return 1;
//     }
    double* pdx = pd + 0;
    double* pdy = pd + 1;
    
    // for each pen point, collect f values in fv
    for (int i = 0; i < np; i++) {
        fv[i] = f(x + (*pdx), y + (*pdy));
        fb[i] = fv[i] < 0;
        pdx += 2;
        pdy += 2;
    }
    
    int ncount = 0;
    for (int i = 0; i < np; i++) {
        if (fb[i])
            ncount += 1;
        if (fb[i] ^ fb[(i + 1) % np])
        {
            double mv = f(x + pdm[i * 2 + 0], y + pdm[i * 2 + 1]);
            bool mb = mv < 0;
            bool crossing_zero = false;
            if (fb[i] == mb)
                crossing_zero = (mb == (fv[i] < mv));
            else
                crossing_zero = (mb != (mv < fv[(i + 1) % np]));
            if (!crossing_zero)
                return 0;
        }
    }
    return (ncount != 0 && ncount != np) ? 1 : 0;
    /*
     * fv[i]  mv  fb[i]  mb  fv[i]<mv  R
     * -5     -4  1      1   1         1
     * -5     -6  1      1   0         0
     *  5      2  0      0   0         1
     *  5      8  0      0   1         0
     * mv  fv[i+1]  mb  fb[i+1]  mv<fv[i+1]  R
     *  2   3       0   0        1           1
     *  6   3       0   0        0           0
     * -1  -3       1   1        0           1
     * -7  -3       1   1        1           0
     */
}

double subdivide(double sx, double sy, int level, double stepx, double stepy, int v00, int v20, int v02, int v22)
{
    int sum = v00 + v20 + v02 + v22;
    if (level > 0 && sum != 0 && sum != 4)
    {
        int v10 = (*get_color)(sx + stepx, sy);
        int v01 = (*get_color)(sx, sy + stepy);
        int v11 = (*get_color)(sx + stepx, sy + stepy);
        int v21 = (*get_color)(sx + stepx * 2, sy + stepy);
        int v12 = (*get_color)(sx + stepx, sy + stepy * 2);
        double stepx2 = stepx * 0.5;
        double stepy2 = stepy * 0.5;
        return (subdivide(sx, sy, level - 1, stepx2, stepy2, v00, v10, v01, v11) +
                subdivide(sx + stepx, sy, level - 1, stepx2, stepy2, v10, v20, v11, v11) +
                subdivide(sx, sy + stepy, level - 1, stepx2, stepy2, v01, v11, v02, v12) +
                subdivide(sx + stepx, sy + stepy, level - 1, stepx2, stepy2, v11, v21, v12, v22)) * 0.25;
    }
    else
        return (double)sum * 0.25;
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
    for (int i = 13; i + 1 < argc; i += 2)
        symbol_table.add_constant(argv[i], atof(argv[i + 1]));
    exprtk::symbol_table<double> unknown_symbol_table;
    expression.register_symbol_table(unknown_symbol_table);
    expression.register_symbol_table(symbol_table);
    parser.enable_unknown_symbol_resolver();
    if (!parser.compile(fx, expression))
    {
        for (std::size_t i = 0; i < parser.error_count(); ++i)
        {
            typedef exprtk::parser_error::type error_t;
            error_t error = parser.get_error(i);
            fprintf(stderr, "error {\"i\": %ld, \"position\": %ld, \"type\": \"%s\", \"message\": \"%s\"\n", 
                    i, error.token.position,
                    exprtk::parser_error::to_str(error.mode).c_str(),
                    error.diagnostic.c_str());
        }
        return 1;
    }    

    bool exit_with_error = false;
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
            fprintf(stderr, "unknown_var %s\n", var_name.c_str());
            exit_with_error = true;
        }
    }
    if (exit_with_error)
        exit(1);
#endif

    pd = (double*)malloc(sizeof(double) * 2 * np);
    pdm = (double*)malloc(sizeof(double) * 2 * np);
    fv = (double*)malloc(sizeof(double) * np);
    fb = (bool*)malloc(sizeof(bool) * np);
    int* scanlines = (int*)malloc(sizeof(int) * (width + 1) * 2);
    int* line0 = scanlines;
    int* line1 = scanlines + width + 1;
    
    if (type == 0)
    {
        get_color = &get_color_line;
        for (int i = 0; i < np; i++)
        {
            pd[i * 2 + 0] = cos((double)i * 2 * M_PI / np) * lw * 0.5 * dx;
            pd[i * 2 + 1] = sin((double)i * 2 * M_PI / np) * lw * 0.5 * dy;
        }
        for (int i = 0; i < np; i++)
        {
            pdm[i * 2 + 0] = (pd[i * 2 + 0] + pd[((i + 1) % np) * 2 + 0]) * 0.5;
            pdm[i * 2 + 1] = (pd[i * 2 + 1] + pd[((i + 1) % np) * 2 + 1]) * 0.5;
        }
    }
    else
        get_color = &get_color_area;
    
    for (int x = 0; x <= width; x++)
        line0[x] = (*get_color)(xl + dx * x, yt);
    int old_percent = -1;
    for (int y = 1; y <= height; y++)
    {
        int percent = y * 100 / height;
        if (percent != old_percent)
        {
            old_percent = percent;
            fprintf(stderr, "progress %d\n", percent);
        }
        for (int x = 0; x <= width; x++)
            line1[x] = (*get_color)(xl + dx * x, yt + dy * y);
        // now we have line0 and line1
        for (int x = 0; x < width; x++)
        {
            unsigned char color = 0;
            int sum = line0[x] + line0[x + 1] + line1[x] + line1[x + 1];
            if (sum == 0)
                color = 0;
            else if (sum == 4)
                color = 255;
            else
                color = round(subdivide(xl + dx * x, yt + dy * (y - 1), aa_level, 0.5 * dx, 0.5 * dy, 
                                        line0[x], line0[x + 1], line1[x], line1[x + 1]) * 255.0);
            fwrite(&color, 1, 1, stdout);
        }
        // swap lines
        int *temp = line0; line0 = line1; line1 = temp;
    }
    
    free(scanlines);
    free(fb);
    free(fv);
    free(pdm);
    free(pd);
    
    return 0;
}

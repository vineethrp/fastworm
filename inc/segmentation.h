#pragma once

#define MAX_PIXEL_VALUE 255

int
greyscale_blur(unsigned char *data, int w, int h,
    int x1, int y1, int x2, int y2, int box_len,
    unsigned char *hblur_data, unsigned char *blur_data);

typedef int (*threshold_fn_t)(unsigned char*, int, int,
    int, int, int, int,
    int, float,
    int*, bool*);

int
simple_threshold(unsigned char *img_data, int w, int h,
    int x1, int y1, int x2, int y2,
    int threshold_winsz, float threshold_ratio,
    int *integrals, bool *threshold);
int
dynamic_threshold(unsigned char *img_data, int w, int h,
    int x1, int y1, int x2, int y2,
    int threshold_winsz, float threshold_ratio,
    int *integrals, bool *threshold);


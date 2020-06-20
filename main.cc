
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#define access _access
#else

#include <unistd.h>

#endif
#define __LINUX__ 1
#define USE_SHELL_OPEN

#include "browse.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"
#include <stdlib.h>
#include <stdio.h>

#include "skin_detection.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

uint8_t *loadImage(const char *filename, int32_t *width, int32_t *height, int32_t *channels) {
    return stbi_load(filename, width, height, channels, 0);
}

void saveImage(const char *filename, int32_t width, int32_t height, int32_t channels, uint8_t *Output) {
    if (!stbi_write_jpg(filename, width, height, channels, Output, 100)) {
        fprintf(stderr, "save file fail.\n");
        return;
    }
#ifdef USE_SHELL_OPEN
    browse(filename);
#endif
}

void splitpath(const char *path, char *drv, char *dir, char *name, char *ext) {
    const char *end;
    const char *p;
    const char *s;
    if (path[0] && path[1] == ':') {
        if (drv) {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    } else if (drv)
        *drv = '\0';
    for (end = path; *end && *end != ':';)
        end++;
    for (p = end; p > path && *--p != '\\' && *p != '/';)
        if (*p == '.') {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++);)
            ext++;
    for (p = end; p > path;)
        if (*--p == '\\' || *p == '/') {
            p++;
            break;
        }
    if (name) {
        for (s = p; s < end;)
            *name++ = *s++;
        *name = '\0';
    }
    if (dir) {
        for (s = path; s < p;)
            *dir++ = *s++;
        *dir = '\0';
    }
}


void rgb2yuv420(uint8_t *rgb_src, uint8_t *y_src, uint8_t *u_src, uint8_t *v_src, int32_t width, int32_t height,
                int32_t channels) {
    for (int32_t y = 0; y < height; y++) {
        uint8_t *scanLine = rgb_src + width * y * channels;
        bool even_y = y % 2 == 0;
        for (int32_t x = 0; x < width; x++) {
            bool even_x = x % 2 == 0;
            uint8_t r = scanLine[0];
            uint8_t g = scanLine[1];
            uint8_t b = scanLine[2];
            *(y_src++) = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            if (even_y && even_x) {
                *(u_src++) = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                *(v_src++) = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
            }
            scanLine += channels;
        }
    }
}

void skin_detection_yuv420(uint8_t *input,
                           uint8_t *output,
                           uint8_t *mask,
                           int32_t width,
                           int32_t height,
                           int32_t channels) {
    const int32_t stride_y = width;
    const int32_t stride_uv = (width + 1) / 2;
    uint8_t *y_src = output;
    uint8_t *u_src = y_src + stride_y * height;
    uint8_t *v_src = u_src + stride_uv * ((height + 1) / 2);
    rgb2yuv420(input, y_src, u_src, v_src, width, height, channels);
    int32_t mb_cols = width >> 4;
    int32_t mb_rows = height >> 4;
    for (int32_t mb_row = 0; mb_row < mb_rows; ++mb_row) {
        for (int32_t mb_col = 0; mb_col < mb_cols; ++mb_col) {
            uint8_t ret = MbHasSkinColor(y_src, u_src, v_src, stride_y, stride_uv, stride_uv, mb_row, mb_col) * 255;
            uint8_t *out = mask + ((mb_row << 4) + 8) * stride_y + (mb_col << 4) + 8;
            out[0] += ret;
            out[1] += ret;
            out[stride_y] += ret;
            out[stride_y + 1] += ret;
        }
    }
}


void rgb2yuv(uint8_t r, uint8_t g, uint8_t b, int32_t *y, int32_t *u, int32_t *v) {
    *y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
    *u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
    *v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
}

void skin_detection_yuv444(uint8_t *rgb_src, uint8_t *out, int32_t width, int32_t height, int32_t channels) {
    int32_t stride = width * channels;
    int32_t lastCol = width * channels - channels;
    int32_t lastRow = height * stride - stride;
    int y00, y10, y01, y11, u00, u10, u01, u11, v00, v10, v01, v11;
    uint8_t y_avg;
    uint8_t u_avg;
    uint8_t v_avg;
    for (int32_t y = 0; y < height; y++) {
        int32_t cur_row = stride * y;
        int32_t next_row = min(cur_row + stride, lastRow);
        uint8_t *next_scanLine = rgb_src + next_row;
        uint8_t *cur_scanLine = rgb_src + cur_row;
        uint8_t *outLine = out + width * y;
        for (int32_t x = 0; x < width; x++) {
            int32_t cur_col = x * channels;
            int32_t next_col = min(cur_col + channels, lastCol);
            uint8_t *c00 = cur_scanLine + cur_col;
            uint8_t *c10 = cur_scanLine + next_col;
            uint8_t *c01 = next_scanLine + cur_col;
            uint8_t *c11 = next_scanLine + next_col;
            rgb2yuv(c00[0], c00[1], c00[2], &y00, &u00, &v00);
            rgb2yuv(c10[0], c10[1], c10[2], &y10, &u10, &v10);
            rgb2yuv(c01[0], c01[1], c01[2], &y01, &u01, &v01);
            rgb2yuv(c11[0], c11[1], c11[2], &y11, &u11, &v11);
            y_avg = ((y00 + y10 + y01 + y11)) >> 2;
            u_avg = ((u00 + u10 + u01 + u11)) >> 2;
            v_avg = ((v00 + v10 + v01 + v11)) >> 2;
            *outLine++ = (SkinPixel(y_avg, u_avg, v_avg) == 1) * 255;
        }
    }
}


int main(int argc, char **argv) {
    printf("Image Processing \n ");
    printf("blog:http://cpuimage.cnblogs.com/ \n ");
    printf("Skin Detection\n ");
    if (argc < 2) {
        printf("usage: \n ");
        printf("%s filename \n ", argv[0]);
        printf("%s image.jpg \n ", argv[0]);
        getchar();
        return 0;
    }
    char *in_file = argv[1];
    if (access(in_file, 0) == -1) {
        printf("load file: %s fail!\n", in_file);
        return -1;
    }
    char drive[3];
    char dir[256];
    char fname[256];
    char ext[256];
    char out_file[1024];
    splitpath(in_file, drive, dir, fname, ext);
    sprintf(out_file, "%s%s%s_out.jpg", drive, dir, fname);

    int32_t width = 0;
    int32_t height = 0;
    int32_t channels = 0;
    uint8_t *input = NULL;
    input = loadImage(in_file, &width, &height, &channels);
    if (input) {
        uint8_t *output = (uint8_t *) calloc(width * channels * height * sizeof(uint8_t), 1);
        if (output) {
            uint8_t *mask = output + width * height * (channels - 1);
            bool apply_yuv420 = false;
            if (apply_yuv420)
                skin_detection_yuv420(input, output, mask, width, height, channels);
            else
                skin_detection_yuv444(input, mask, width, height, channels);
            saveImage(out_file, width, height, 1, mask);
            free(output);
        }
        free(input);
    } else {
        printf("load file: %s fail!\n", in_file);
    }
    printf("press any key to exit. \n");
    getchar();
    return 0;
}
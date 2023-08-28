#include <stdio.h>
#include <Windows.h>


#include <sys/stat.h>

int X = 40; // HEIGHT
int Y = 60; // WIDTH
const int DPI = 90;
const int ZOOM = 4;

struct bitmap {
    float r, g, b;
};

struct bitmap * loadBitmap(char * filename, BITMAPINFOHEADER * bitmapInfoHeader) {

    FILE * filePtr;
    BITMAPFILEHEADER bitmapFileHeader;
    unsigned char * bitmapImage;
    int imageIdx = 0;
    unsigned char tempRGB;

    // Open BMP file
    filePtr = fopen(filename, "rb");
    if (filePtr == NULL) {
        printf("Error reading input file!");
        return NULL;
    }
    fread( & bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
    if (bitmapFileHeader.bfType != 0x4D42) { fclose(filePtr); return NULL; }

    // Read header and bitmap
    fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
    fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);
    bitmapImage = (unsigned char * ) malloc(bitmapInfoHeader->biSizeImage);
    if (!bitmapImage) {
        free(bitmapImage);
        fclose(filePtr);
        return NULL;
    }

    X = bitmapInfoHeader->biHeight;
    Y = bitmapInfoHeader->biWidth;

    printf("%d %d\n", X, Y);
    // Initing pixels
    fread(bitmapImage, bitmapInfoHeader->biSizeImage, 1, filePtr);
    struct bitmap *pixels = (struct bitmap *) malloc(X * Y * sizeof(struct bitmap));

    // Line by line from bottom
    int x = X - 1; int y = 0;
    for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx += 3) {

        tempRGB = bitmapImage[imageIdx];
        bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
        bitmapImage[imageIdx + 2] = tempRGB;

        struct bitmap t = {(int)bitmapImage[imageIdx], (int)bitmapImage[imageIdx], (int)bitmapImage[imageIdx]};
        pixels[x * Y + y] = t;
        y += 1;
        if (y == Y) {
            y = 0;
            x -= 1;
        }

    }

    fclose(filePtr);
    return pixels;

}


void saveBitmap(const char *fn, int h, int w, int dpi, struct bitmap *data) {

    FILE * image;
    int image_size = w * h;
    int file_size = 54 + 4 * image_size;
    int ppm = dpi * 39.375;

    struct bitmap_file_header {
        unsigned char bitmap_type[2]; // 2 bytes
        int file_size; // 4 bytes
        short reserved1; // 2 bytes
        short reserved2; // 2 bytes
        unsigned int offset_bits; // 4 bytes
    };

    struct bitmap_image_header {
        unsigned int size_header; // 4 bytes
        unsigned int w; // 4 bytes
        unsigned int h; // 4 bytes
        short int planes; // 2 bytes
        short int bit_count; // 2 bytes
        unsigned int compression; // 4 bytes
        unsigned int image_size; // 4 bytes
        unsigned int ppm_x; // 4 bytes
        unsigned int ppm_y; // 4 bytes
        unsigned int clr_used; // 4 bytes
        unsigned int clr_important; // 4 bytes
    };

   struct bitmap_file_header bfh;
   struct bitmap_image_header bih;

    memcpy( & bfh.bitmap_type, "BM", 2);
    bfh.file_size = file_size;
    bfh.reserved1 = 0;
    bfh.reserved2 = 0;
    bfh.offset_bits = 0;

    bih.size_header = sizeof(bih);
    bih.w = w;
    bih.h = h;
    bih.planes = 1;
    bih.bit_count = 24;
    bih.compression = 0;
    bih.image_size = file_size;
    bih.ppm_x = ppm;
    bih.ppm_y = ppm;
    bih.clr_used = 0;
    bih.clr_important = 0;

    image = fopen(fn, "wb");

    if (image == NULL) {
        printf("Error writing output file!");
        return;
    }

    fwrite( & bfh, 1, 14, image);
    fwrite( & bih, 1, sizeof(bih), image);

    int x = h - 1;
    int y = 0;
    while (x >= 0) {
        struct bitmap BGR = data[x * w + y];

        float red = (BGR.r);
        float green = (BGR.g);
        float blue = (BGR.b);

        unsigned char color[3] = { blue, green, red };

        fwrite(color, 1, sizeof(color), image);

        y += 1;
        if (y == w) {
            y = 0;
            x -= 1;
        }

    }

    fclose(image);

}

struct bitmap * fieldToBitmap(int * field, int zoom) {
    struct bitmap *pixels = (struct bitmap *) malloc(X * Y * zoom * zoom * sizeof(struct bitmap));

    for (int x = 0; x < X * zoom; x++) {
        for (int y = 0; y < Y * zoom; y++) {
            int i = x * Y * ZOOM + y;
            int t = field[(int)(x/zoom) * Y + (int)(y/zoom)] == 0 ? 255 : 0;
            pixels[i].r = t;
            pixels[i].g = t;
            pixels[i].b = t;
        }
    }
    return pixels;
}

int * bitmapToField(struct bitmap * pixels) {
    int * field = (int *) malloc(X * Y * sizeof(int));

    for (int x = 0; x < X; x++) {
        for (int y = 0; y < Y; y++) {
            int i = x * Y + y;

            int t = (pixels[i].r < 127) ? 1 : 0;
            field[i] = t;
        }
    }
    return field;
}

int nextStep(int * currentField) {

    int * nextField = (int *) malloc(X * Y * sizeof(int));
    for (int i = 0; i < X; i++) {
        for (int j = 0; j < Y; j++) {

            int cnt = 0;
            for (int k = 0; k < 9; k++) {

                if (k != 4) {
                    int kx = (i - 1 + (int)(k / 3));
                    int ky = (j - 1 + k % 3);

                    //printf("%i %i %d %d\n", i, j, kx, ky);

                    if ((kx >= 0) && (kx < X) && (ky >= 0) && (ky < Y)) {
                        if (currentField[ kx * Y + ky] == 1) {
                            cnt++;
                        }
                    }
                }

            }

            //printf("%i\n", cnt);

            nextField[i * Y + j] = currentField[i * Y + j];

            if (currentField[i * Y + j] == 0) {
                if (cnt == 3) {
                    nextField[i * Y + j] = 1;
                }
            } else {
                if ((cnt != 2) && (cnt != 3)) {
                    nextField[i * Y + j] = 0;
                }
            }

        }
    }

    for (int i = 0; i < X; i++) {
        for (int j = 0; j < Y; j++) {
            currentField[i * Y + j] = (int) nextField[i * Y + j];
        }
    }

    free(nextField);

}

void playGame(int * field, char * outputDir, int k, int freq) {

    mkdir(outputDir);

    int * prevField = (int *) malloc(X * Y * sizeof(int));
    for (int i = 0; i < X; i++) {
        for (int j = 0; j < Y; j++) {
            prevField[i * Y + j] = field[i * Y + j];
        }
    }

    for (int i = 0; i < k * freq; i++) {

        if (i % freq == 0) {
            struct bitmap * bmp = fieldToBitmap(field, ZOOM);

            char buf[100];
            snprintf(buf, 100, "%s/output%d.bmp", outputDir, i / freq);
            saveBitmap(buf, X * ZOOM, Y * ZOOM, DPI, bmp);
            free(bmp);
        }

        nextStep(field);

        int difference = 0;
        for (int i = 0; i < X; i++) {
            for (int j = 0; j < Y; j++) {
                if (prevField[i * Y + j] != field[i * Y + j]) {
                    difference = 1;
                }
            }
        }

        if (!difference) {
            break;
        }

        if (i != 0){
            nextStep(prevField);
        }

    }

}

int main(int argc, char **argv) {

    BITMAPINFOHEADER * bitmapInfoHeader = (BITMAPINFOHEADER *) malloc(sizeof(BITMAPINFOHEADER));
    int * field;
    char output[100];
    int k = INT_MAX;
    int freq = 1;

    if (argc > 1) {

        FILE* tar;

        for (int i = 1; i < argc; ++i) {

            if (strcmp(argv[i], "--input") == 0) {
                printf("%s\n", argv[i+1]);
                field = bitmapToField(loadBitmap(argv[i+1], bitmapInfoHeader));
                i++;
                continue;
            }

            if (strcmp(argv[i], "--output") == 0) {
                strcpy(output, argv[i+1]);
                i++;
                continue;
            }

            if (strcmp(argv[i], "--max_iter") == 0) {
                k = atoi(argv[i+1]);
                i++;
                continue;
            }

            if (strcmp(argv[i], "--freq") == 0) {
                freq = atoi(argv[i+1]);
                i++;
                continue;
            }

        }

        playGame(field, output, k, freq);
        free(field);

    }

    return 0;

}

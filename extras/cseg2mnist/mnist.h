#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <colib.h>

static const int MNIST_UBYTE = 0x08;        // 1byte
static const int MNIST_SBYTE = 0x09;        // 1byte
static const int MNIST_SHORT = 0x0B;        // 2bytes
static const int MNIST_INT = 0x0C;          // 4bytes
static const int MNIST_FLOAT = 0x0D;        // 4bytes
static const int MNIST_DOUBLE = 0x0E;       // 8bytes

void writeMnist(char x, FILE *file) {
    fputc(x, file);
}

void writeMnist(unsigned char x, FILE *file) {
    fputc(x, file);
}

void writeMnist(int32_t x, FILE *file) {
    fputc(x >> 24, file);
    fputc(x >> 16, file);
    fputc(x >>  8, file);
    fputc(x,       file);
}

void writeMnist(float x, FILE *file) {
    fwrite(&x, 4, 1, file);
}

void writeMnist(colib::bytearray& data, FILE* file) {
    for(int y=data.dim(1)-1; y>=0; y--) {
        for(int x=0; x<data.dim(0); x++) {
            fputc(data(x,y), file);
        }
    }
}

void writeMnist(colib::intarray& data, FILE* file) {
    for(int y=data.dim(1)-1; y>=0; y--) {
        for(int x=0; x<data.dim(0); x++) {
            fputc((data(x,y)>>16) & 0xFF, file);
            fputc((data(x,y)>>8) & 0xFF, file);
            fputc(data(x,y) & 0xFF, file);
        }
    }
}

FILE* openMnist(const char* filename, int nDim, int dataType = MNIST_UBYTE) {
    FILE* mnist = fopen(filename, "w");
    writeMnist((dataType<<8)+nDim, mnist);
    writeMnist(0, mnist);
    return mnist;
}

void closeMnist(FILE* mnist, int n) {
    fseek(mnist, 4, SEEK_SET);
    writeMnist(n, mnist);
    fclose(mnist);
}


#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

struct tagBITMAPFILEHEADER get_file_header(char *fileName);
struct tagBITMAPINFOHEADER get_info_header(char *fileName);
unsigned char *get_pixel_data(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, unsigned char *location);
void write_bmp(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, unsigned char *pixels);
unsigned char *brighten(unsigned char *pixels, int start, int stop, float brightness);

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
struct tagBITMAPFILEHEADER
{
    WORD bfType;      //specifies the file type
    DWORD bfSize;     //specifies the size in bytes of the bitmap file
    WORD bfReserved1; //reserved; must be 0
    WORD bfReserved2; //reserved; must be 0
    DWORD bfOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
};
struct tagBITMAPINFOHEADER
{
    DWORD biSize;         //specifies the number of bytes required by the struct
    LONG biWidth;         //specifies width in pixels
    LONG biHeight;        //species height in pixels
    WORD biPlanes;        //specifies the number of color planes, must be 1
    WORD biBitCount;      //specifies the number of bit per pixel
    DWORD biCompression;  //spcifies the type of compression
    DWORD biSizeImage;    //size of image in bytes
    LONG biXPelsPerMeter; //number of pixels per meter in x axis
    LONG biYPelsPerMeter; //number of pixels per meter in y axis
    DWORD biClrUsed;      //number of colors used by th ebitmap
    DWORD biClrImportant; //number of colors that are important
};

int main(int argc, char *argv[])
{
    //load argument values into variables
    float brightness = (float)atof(argv[2]);
    int parallel = atof(argv[3]);
    //create file headers for the bmp file
    struct tagBITMAPFILEHEADER fileHeader = get_file_header(argv[1]);
    struct tagBITMAPINFOHEADER infoHeader = get_info_header(argv[1]);
    //begin the clock
    clock_t start = clock();
    //use mmap to allocate shared memory in size of pixel stream
    int size = infoHeader.biHeight * infoHeader.biWidth * 3;
    unsigned char *brightened = (unsigned char *)mmap(NULL, size * sizeof(unsigned char), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    //load input file data into brightened pointer
    brightened = get_pixel_data(argv[1], infoHeader, fileHeader, brightened);

    //check if we are doing a parallel split
    if (parallel == 1)
    {
        //calculate the halfway point
        int half = size / 2;
        if (fork() == 0)
        {
            //run half of the process in the child function
            brightened = brighten(brightened, half, size, brightness);
            return 0;
        }
        else
        {
            //run other half in parent process and wait for child to finish
            brightened = brighten(brightened, 0, half, brightness);
            wait(0);
        }
    }
    else
    {
        //run with only one process 
        brightened = brighten(brightened, 0, size, brightness);
    }
    
    //end the clock and print times
    clock_t end = clock(); 
    printf("%ld \n", end - start);
    double timediff = (double)(end - start) / CLOCKS_PER_SEC;
    printf("%f \n", timediff);
    //write new stream to output file and free data
    write_bmp(argv[4], infoHeader, fileHeader, brightened);
    munmap(brightened, size);
    return 0;
}

unsigned char *brighten(unsigned char *pixels, int start, int stop, float brightness)
{
    for (int i = start; i != stop; i++)
    {
        int newBright = pixels[i] + 255 * brightness;
        
        //make sure new value is valid
        if (newBright > 255)
            newBright = 255;
        pixels[i] = (unsigned char)newBright;
    }
    return pixels;
}

struct tagBITMAPFILEHEADER get_file_header(char *fileName)
{
    //initialize struct to be loaded into
    struct tagBITMAPFILEHEADER header;
    //open and check file
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        exit(0);
    }
    //read file type and confirm of type bmp
    fread(&(header.bfType), sizeof(unsigned char) * 2, 1, fp);
    if (header.bfType != 19778)
    {
        exit(0);
    }
    //read rest of data for header and close file
    fread(&(header.bfSize), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.bfReserved1), sizeof(unsigned char) * 2, 1, fp);
    fread(&(header.bfReserved2), sizeof(unsigned char) * 2, 1, fp);
    fread(&(header.bfOffBits), sizeof(unsigned char) * 4, 1, fp);
    fclose(fp);
    return header;
}

struct tagBITMAPINFOHEADER get_info_header(char *fileName)
{
    //initialize struct to be loaded into
    struct tagBITMAPINFOHEADER header;
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        exit(0);
    }
    fseek(fp, sizeof(unsigned char) * 14, SEEK_SET);

    fread(&(header.biSize), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biWidth), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biHeight), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biPlanes), sizeof(unsigned char) * 2, 1, fp);
    fread(&(header.biBitCount), sizeof(unsigned char) * 2, 1, fp);
    fread(&(header.biCompression), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biSizeImage), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biXPelsPerMeter), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biYPelsPerMeter), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biClrUsed), sizeof(unsigned char) * 4, 1, fp);
    fread(&(header.biClrImportant), sizeof(unsigned char) * 4, 1, fp);
    fclose(fp);
    return header;
}

unsigned char *get_pixel_data(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, unsigned char *location)
{
    //
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        exit(0);
    }
    //obtain offset of pixel data, use offset to find it with fseek
    int offset = fileHeader.bfOffBits;
    fseek(fp, offset, SEEK_SET);
    //use data size to allocate data for pixels
    int dataSize = infoHeader.biWidth * infoHeader.biHeight * 3;
    //read data into ptr pixels
    fread(location, dataSize, 1, fp);
    fclose(fp);
    return location;
}

void write_bmp(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, unsigned char *pixels)
{
    FILE *fp = fopen(fileName, "wb");
    if (fp == NULL)
    {
        exit(0);
    }
    //write info header first
    fwrite(&(fileHeader.bfType), sizeof(unsigned char) * 2, 1, fp);
    fwrite(&(fileHeader.bfSize), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(fileHeader.bfReserved1), sizeof(unsigned char) * 2, 1, fp);
    fwrite(&(fileHeader.bfReserved2), sizeof(unsigned char) * 2, 1, fp);
    fwrite(&(fileHeader.bfOffBits), sizeof(unsigned char) * 4, 1, fp);

    //write image header next
    fwrite(&(infoHeader.biSize), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biWidth), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biHeight), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biPlanes), sizeof(unsigned char) * 2, 1, fp);
    fwrite(&(infoHeader.biBitCount), sizeof(unsigned char) * 2, 1, fp);
    fwrite(&(infoHeader.biCompression), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biSizeImage), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biXPelsPerMeter), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biYPelsPerMeter), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biClrUsed), sizeof(unsigned char) * 4, 1, fp);
    fwrite(&(infoHeader.biClrImportant), sizeof(unsigned char) * 4, 1, fp);

    //write image data
    fwrite(pixels, infoHeader.biSizeImage, 1, fp);
    fclose(fp);
}

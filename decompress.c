#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
//Cole Robinson, Midterm1

typedef unsigned char BYTE;

typedef struct col
{
    int r, g, b; //red, green and blue, in that order.  The values will not exceed 255!
} col;

typedef struct compressedformat
{
    int width, height;      //width and height of the image, with one byte for each color, blue, green and red
    int rowbyte_quarter[4]; //for parallel algorithms! That’s the location in bytes which exactly splits the result image after decompression into 4 equal parts!
    int palettecolors;      //how many colors does the picture have?
    col colors[];           //all participating colors of the image. Further below is the structure “col” described
} compressedformat;

typedef struct chunk
{
    BYTE color_index; //which of the color of the palette
    short count;      //How many pixel of the same color from color_index are continuously appearing
} chunk;

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

compressedformat *readcompressedheader(char *fileName);
struct tagBITMAPFILEHEADER createfileheader();
struct tagBITMAPINFOHEADER createinfoheader();
int readcompresseddata(int start, int stop, BYTE *output, compressedformat *header, char *fileName);
void write_bmp(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, BYTE *data1, int length1,
               BYTE *data2, int length2, BYTE *data3, int length3, BYTE *data4, int length4);

int main(int argc, char *argv[])
{
    char *fileName = "compressed.bin"; //variable for different files
    char *outputFile = "decompressed.bmp"; //variable for different outputs files

    compressedformat *header = readcompressedheader(fileName);
    struct tagBITMAPFILEHEADER fileHeader = createfileheader();
    struct tagBITMAPINFOHEADER infoHeader = createinfoheader();
    //Since we are splitting the data into 4 parts, we will have four shared memory arrays
    //the write function will then use all of these arrays to create the output file
    //each will have a sufficient size of 1/3 the total file size
    int forkedSize = (header->height * header->width);

    BYTE *data1 = (BYTE *)mmap(NULL, forkedSize, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    BYTE *data2 = (BYTE *)mmap(NULL, forkedSize, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    BYTE *data3 = (BYTE *)mmap(NULL, forkedSize, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    BYTE *data4 = (BYTE *)mmap(NULL, forkedSize, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    //lengths will hold the number of bytes written into these mmap data arrays, will be used to know how many bytes to write.
    int *lengths = (int *)mmap(NULL, sizeof(int) * 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    //check for allocation
    if (data1 == NULL | data2 == NULL | data3 == NULL | data4 == NULL | lengths == NULL)
        exit(0);
    //first start will hold the location in bytes of the first chunk
    int firststart = sizeof(compressedformat) + sizeof(col) * header->palettecolors;

    if (fork() == 0)
    {
        //add data to each Byte pointer in each fork
        lengths[1] = readcompresseddata((header->rowbyte_quarter)[0] + 1, (header->rowbyte_quarter)[1], data2, header, fileName);
        if (fork() == 0)
        {
            lengths[2] = readcompresseddata((header->rowbyte_quarter)[1] + 1, (header->rowbyte_quarter)[2], data3, header, fileName);
            if (fork() == 0)
            {
                lengths[3] = readcompresseddata((header->rowbyte_quarter)[2] + 1, 
                        (header->rowbyte_quarter)[3] + sizeof(compressedformat) + sizeof(col) * header->palettecolors, data4, header, fileName);
                return 0;
            }
            wait(0);
            return 0;
        }
        wait(0);
        return 0;
    }
    else
    {
        lengths[0] = readcompresseddata(firststart, (header->rowbyte_quarter)[0], data1, header, fileName);
        wait(0);
        //write to  output file
        write_bmp(outputFile, infoHeader, fileHeader, data1, lengths[0], data2, lengths[1], data3, lengths[2], data4, lengths[3]);
        //free memory
        munmap(data1, forkedSize);
        munmap(data2, forkedSize);
        munmap(data3, forkedSize);
        munmap(data4, forkedSize);
        munmap(lengths, sizeof(int) * 4);
        munmap(header, sizeof(compressedformat) + sizeof(col) * header->palettecolors); //maybe?
        return 0;
    }
}

compressedformat *readcompressedheader(char *fileName)
{
    //open file for reading in binary
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
        exit(0);

    //create and read into header
    compressedformat header;
    fread(&(header.width), sizeof(int), 1, fp);
    fread(&(header.height), sizeof(int), 1, fp);
    fread(&(header.rowbyte_quarter), sizeof(int) * 4, 1, fp);
    fread(&(header.palettecolors), sizeof(int), 1, fp);
    //create a separte col array, this seemed to be the only way for me to do this
    col test[header.palettecolors];
    fread(&test, sizeof(col) * header.palettecolors, 1, fp);
    //create a new header that has allocated memory to account for the flexible array in struct compressedformat
    compressedformat *newHeader = (compressedformat *)mmap(NULL, sizeof(compressedformat) + sizeof(col) * header.palettecolors, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    newHeader->width = header.width;
    newHeader->height = header.height;
    for (int i = 0; i < 4; i++)
    {
        (newHeader->rowbyte_quarter)[i] = (header.rowbyte_quarter)[i];
    }
    newHeader->palettecolors = header.palettecolors;

    for (int i = 0; i < (newHeader->palettecolors); i++)
    {
        (newHeader->colors)[i] = test[i];
    }
    fclose(fp);
    //assign values to new header and return
    return newHeader;
}

int readcompresseddata(int start, int stop, BYTE *output, compressedformat *header, char *fileName)
{
    //open file
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
        exit(0);
    //move pointer to desired location
    fseek(fp, sizeof(BYTE) * start, SEEK_SET);
    //allocate memory for a chunk
    chunk *head = malloc(sizeof(chunk));
    int i = 0; //i will hold the number of bytes being written to output
    while (stop > start)
    {
        fread(&(head->color_index), sizeof(BYTE), 1, fp);
        fread(&(head->count), sizeof(short), 1, fp);
        if (feof(fp) != 0)
            break;
        short count = head->count;
        BYTE index = head->color_index;
        //assign values to the shared output Byte array
        while (count > 0)
        {
            output[i] = (header->colors[index]).b;
            i++;
            output[i] = (header->colors[index]).g;
            i++;
            output[i] = (header->colors[index]).r;
            i++;
            count--;
        }
        //advance to next chunk
        start = start + sizeof(BYTE) + sizeof(short);
    }
    free(head);
    fclose(fp);
    return i;
}

struct tagBITMAPFILEHEADER createfileheader()
{
    struct tagBITMAPFILEHEADER file;
    file.bfType = (WORD)19778;
    file.bfSize = (DWORD)4320054;
    file.bfReserved1 = (WORD)0;
    file.bfReserved2 = (WORD)0;
    file.bfOffBits = (DWORD)54;
    return file;
}

struct tagBITMAPINFOHEADER createinfoheader()
{
    struct tagBITMAPINFOHEADER info;
    info.biSize = (DWORD)40;
    info.biWidth = (LONG)1200;
    info.biHeight = (LONG)1200;
    info.biPlanes = (WORD)1;
    info.biBitCount = (WORD)24;
    info.biCompression = (DWORD)0;
    info.biSizeImage = (DWORD)4320000;
    info.biXPelsPerMeter = (LONG)3780;
    info.biYPelsPerMeter = (LONG)3780;
    info.biClrUsed = (DWORD)0;
    info.biClrImportant = (DWORD)0;
    return info;
}
void write_bmp(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, BYTE *data1, int length1,
               BYTE *data2, int length2, BYTE *data3, int length3, BYTE *data4, int length4)
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
    fwrite(data1, sizeof(BYTE), length1, fp);
    fwrite(data2, sizeof(BYTE), length2, fp);
    fwrite(data3, sizeof(BYTE), length3, fp);
    fwrite(data4, sizeof(BYTE), length4, fp);
    fclose(fp);
}

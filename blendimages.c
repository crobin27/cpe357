#include <stdio.h>
#include <stdlib.h>
//Program1, Cole Robinson

//function declarations
struct tagBITMAPFILEHEADER get_file_header(char *fileName);
struct tagBITMAPINFOHEADER get_info_header(char *fileName);
unsigned char *get_pixel_data(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader);
unsigned char *blend(unsigned char *p1, unsigned char *p2, float ratio, int size);
void write_bmp(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, unsigned char *pixels);
void printMan();

//structs used for storing header info
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
    if (argc != 5)
    {
        printMan();
        exit(0);
    }
    float ratio = (float)atof(argv[3]);
    if (ratio > 1 || ratio < 0)
    {
        printMan();
        exit(0);
    }

    struct tagBITMAPFILEHEADER fileHeader1, fileHeader2;
    struct tagBITMAPINFOHEADER infoHeader1, infoHeader2;
    //load data into structs and data stream for file1
    fileHeader1 = get_file_header(argv[1]);
    infoHeader1 = get_info_header(argv[1]);
    unsigned char *p1 = get_pixel_data(argv[1], infoHeader1, fileHeader1);
    //repeat for file2
    fileHeader2 = get_file_header(argv[2]);
    infoHeader2 = get_info_header(argv[2]);
    unsigned char *p2 = get_pixel_data(argv[2], infoHeader2, fileHeader2);

    //get sizes of both images
    int size1 = infoHeader1.biHeight * infoHeader1.biWidth * 3;
    int size2 = infoHeader2.biWidth * infoHeader2.biHeight * 3;
    unsigned char *blended; //this will be used to hold blended pixels
    if (size1 == size2)
    {
        blended = (unsigned char *)malloc(size1);
        blended = blend(p1, p2, ratio, size1);
    }
    else
    {
        printf("Only blending of same resolution is supported at this time\n");
        exit(0);
    }
    //write data into target file
    write_bmp(argv[4], infoHeader1, fileHeader1, blended);
    return 0;
}

struct tagBITMAPFILEHEADER get_file_header(char *fileName)
{
    //initialize struct to be loaded into
    struct tagBITMAPFILEHEADER header;
    //open and check file
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printMan();
        exit(0);
    }
    //read file type and confirm of type bmp
    fread(&(header.bfType), sizeof(unsigned char) * 2, 1, fp);
    if (header.bfType != 19778)
    {
        printMan();
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
        printMan();
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

unsigned char *get_pixel_data(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader)
{
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printMan();
        exit(0);
    }
    //obtain offset of pixel data, use offset to find it with fseek
    int offset = fileHeader.bfOffBits;
    fseek(fp, offset, SEEK_SET);
    //use data size to allocate data for pixels
    int dataSize = infoHeader.biWidth * infoHeader.biHeight * 3;
    unsigned char *pixels = NULL;
    pixels = (unsigned char *)malloc(dataSize);
    //read data into ptr pixels
    fread(pixels, dataSize, 1, fp);
    fclose(fp);
    return pixels;
}

unsigned char *blend(unsigned char *p1, unsigned char *p2, float ratio, int size)
{
    unsigned char *ptr;
    //create array of size of resulting image
    unsigned char blended[size];
    ptr = blended;
    unsigned char temp;
    for (int index = 0; index != size; index++)
    {
        temp = (*p1) * ratio + (*p2) * (1 - ratio);
        blended[index] = temp;
        p1++;
        p2++;
    }
    return ptr;
}

void write_bmp(char *fileName, struct tagBITMAPINFOHEADER infoHeader, struct tagBITMAPFILEHEADER fileHeader, unsigned char *pixels)
{
    FILE *fp = fopen(fileName, "wb");

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
    fwrite(pixels, infoHeader.biWidth * infoHeader.biHeight * 3, 1, fp);
    fclose(fp);
}

void printMan()
{
    printf("There was an error with one of the inputs you entered\n");
    printf("Remember the format for using blendimages is as below: \n");
    printf("Command Line: [programname] [imagefile1] [imagefile2] [ratio] [outputfile] \n\n");
    printf("Other Reminders: \n");
    printf("1. Remember all files must be of type bmp, including the file output\n");
    printf("2. The ratio must be a float between 0 and 1\n");
    printf("3. Errors in opening the bmp files will also result in this page being printed\n");
}

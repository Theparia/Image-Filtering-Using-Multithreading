#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <pthread.h>
#include <algorithm>

using namespace std;

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
// #pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

struct RGB
{
  unsigned char blue;
  unsigned char green;
  unsigned char red;
};

vector<vector<RGB>> pixels;

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File " << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          pixels[i][j].red = fileReadBuffer[end - count];
          break;
        case 1:
          pixels[i][j].green = fileReadBuffer[end - count];
          break;
        case 2:
          pixels[i][j].blue = fileReadBuffer[end - count];
          break;
        }
        count++;
      }
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          fileBuffer[bufferSize - count] = pixels[i][j].red;
          break;
        case 1:
          fileBuffer[bufferSize - count] = pixels[i][j].green;
          break;
        case 2:
          fileBuffer[bufferSize - count] = pixels[i][j].blue;
          break;
        }
        count++;
      }
  }
  write.write(fileBuffer, bufferSize);
}

vector<float> getMean()
{
  vector<float> mean;
  float sum_red = 0, sum_green = 0, sum_blue = 0;
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      sum_red += pixels[i][j].red;
      sum_green += pixels[i][j].green;
      sum_blue += pixels[i][j].blue;
    }
  }
  mean.push_back(sum_red / (rows * cols));
  mean.push_back(sum_green / (rows * cols));
  mean.push_back(sum_blue / (rows * cols));
  return mean;
}

void smoothingFilter(int row_start, int row_end)
{
  row_start = max(row_start, 1);
  row_end = min(rows - 1, row_end);
  for (int i = row_start; i < row_end; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      float red_sum = 0, green_sum = 0, blue_sum = 0;
      for (int k = i - 1; k <= i + 1; k++)
      {
        for (int z = j - 1; z <= j + 1; z++)
        {
          red_sum += pixels[k][z].red;
          green_sum += pixels[k][z].green;
          blue_sum += pixels[k][z].blue;
        }
      }
      pixels[i][j].red = red_sum / 9;
      pixels[i][j].green = green_sum / 9;
      pixels[i][j].blue = blue_sum / 9;
    }
  }
}

void sepiaFilter(int row_start, int row_end)
{
  for (int i = row_start; i < row_end; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      int red = (pixels[i][j].red * 0.393) + (pixels[i][j].green * 0.769) + (pixels[i][j].blue * 0.189);
      int green = (pixels[i][j].red * 0.349) + (pixels[i][j].green * 0.686) + (pixels[i][j].blue * 0.168);
      int blue = (pixels[i][j].red * 0.272) + (pixels[i][j].green * 0.534) + (pixels[i][j].blue * 0.131);
      pixels[i][j].red = min(255, red);
      pixels[i][j].green = min(255, green);
      pixels[i][j].blue = min(255, blue);
    }
  }
}

void washedOutFilter(vector<float> mean, int row_start, int row_end)
{
  for (int i = row_start; i < row_end; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      pixels[i][j].red = pixels[i][j].red * 0.4 + mean[0] * 0.6;
      pixels[i][j].green = pixels[i][j].green * 0.4 + mean[1] * 0.6;
      pixels[i][j].blue = pixels[i][j].blue * 0.4 + mean[2] * 0.6;
    }
  }
}

void crossFilter()
{
  int points[4][2] = {{0, 0}, {0, cols - 1}, {rows - 1, 0}, {rows - 1, cols - 1}};

  for(int i = 0; i < 4; i++)
  {
      pixels[points[i][0]][points[i][1]].red = 255;
      pixels[points[i][0]][points[i][1]].green = 255;
      pixels[points[i][0]][points[i][1]].blue = 255;
  }

  for (int i = 1; i < rows - 1; i++)
  {
    for (int j = -1; j <= 1; j++)
    {
      pixels[i][i + j].red = 255;
      pixels[i][i + j].green = 255;
      pixels[i][i + j].blue = 255;
      pixels[i][rows - i - 1 + j].red = 255;
      pixels[i][rows - i - 1 + j].green = 255;
      pixels[i][rows - i - 1 + j].blue = 255;
    }
  }
}

struct thread_data
{
  int row_start;
  int row_end;
  vector<float> mean;
};

void *ApplySmoothingAndSepiaFilters(void *threadarg)
{
  struct thread_data *my_data;
  my_data = (struct thread_data *)threadarg;
  int row_start = my_data->row_start;
  int row_end = my_data->row_end;
  smoothingFilter(row_start, row_end);
  sepiaFilter(row_start, row_end);
  pthread_exit(NULL);
}

void *ApplyWashedOutFilter(void *threadarg)
{
  struct thread_data *my_data;
  my_data = (struct thread_data *)threadarg;
  int row_start = my_data->row_start;
  int row_end = my_data->row_end;
  vector<float> mean = my_data->mean;
  washedOutFilter(mean, row_start, row_end);
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  auto start = std::chrono::high_resolution_clock::now();

  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];

  auto started = std::chrono::high_resolution_clock::now();
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }

  pixels.resize(rows, vector<RGB>(cols));
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);

  pthread_t threads[8];
  struct thread_data td[8];
  int rc;
  void *status;
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  int dim[4][4] = {{0, rows / 4}, {rows / 4, rows * 2 / 4}, {rows * 2 / 4, rows * 3 / 4}, {rows * 3 / 4, rows}};

  for (int i = 0; i < 4; i++)
  {
    td[i].row_start = dim[i][0];
    td[i].row_end = dim[i][1];
    pthread_create(&threads[i], &attr, ApplySmoothingAndSepiaFilters, (void *)&td[i]);
  }

  pthread_attr_destroy(&attr);
  for (int i = 0; i < 4; i++)
  {
    pthread_join(threads[i], &status);
  }

  vector<float> mean = getMean();

  for (int i = 4; i < 8; i++)
  {
    td[i].row_start = dim[i - 4][0];
    td[i].row_end = dim[i - 4][1];
    td[i].mean = mean;
    pthread_create(&threads[i], &attr, ApplyWashedOutFilter, (void *)&td[i]);
  }

  pthread_attr_destroy(&attr);
  for (int i = 4; i < 8; i++)
  {
    pthread_join(threads[i], &status);
  }

  crossFilter();
  writeOutBmp24(fileBuffer, "output.bmp", bufferSize);

  auto end = std::chrono::high_resolution_clock::now();
  cout << "Execution Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  return 0;
}
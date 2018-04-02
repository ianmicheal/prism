#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifndef SEEK_SET
#define 	SEEK_SET   0
#define 	SEEK_CUR   1
#define 	SEEK_END   2
#endif

#ifdef DREAMCAST

typedef int FileHandler;

#elif defined _WIN32 || defined __EMSCRIPTEN__

#define O_RDONLY	0x1
#define O_WRONLY	0x2

#define FILEHND_INVALID	0

typedef FILE* FileHandler;

#endif

typedef struct {
  int mIsOwned;
  uint32_t mLength;
  void* mData;
} Buffer;

typedef char* BufferPointer;

FileHandler fileOpen(char* tPath, int tFlags);
int fileClose(FileHandler tHandler);
size_t fileRead(FileHandler tHandler, void* tBuffer, size_t tCount);
size_t fileWrite(FileHandler tHandler, const void* tBuffer, size_t tCount);
size_t fileSeek(FileHandler tHandler, size_t tOffset, int tWhence);
size_t fileTell(FileHandler tHandler);
size_t fileTotal(FileHandler tHandler);
int fileUnlink(char* tPath);
void* fileMemoryMap(FileHandler tHandler);

int isFile(char* tPath);
int isDirectory(char* tPath);

Buffer makeBuffer(void* tData, uint32_t tLength);
Buffer makeBufferOwned(void* tData, uint32_t tLength);
Buffer fileToBuffer(char* path);
void bufferToFile(char* tPath, Buffer tBuffer);
void freeBuffer(Buffer buffer);
void appendTerminationSymbolToBuffer(Buffer* tBuffer);
void fileToMemory(void* tDst, int tSize, char* tPath);
BufferPointer getBufferPointer(Buffer tBuffer);
void readFromBufferPointer(void* tDst, BufferPointer* tPointer, uint32_t tSize);
int readIntegerFromTextStreamBufferPointer(BufferPointer* tPointer);

void initFileSystem();
void setFileSystem(char* path);
void setWorkingDirectory(char* path);
const char* getFileSystem();
const char* getWorkingDirectory();

void mountRomdiskFromBuffer(Buffer b, char* tMountPath);
void mountRomdisk(char* tFilePath, char* tMountPath);
void unmountRomdisk(char* tMountPath);

char* getPureFileName(char* path);
char* getFileExtension(char* tPath);
void getPathWithoutFileExtension(char* tDest, char* tPath);
void  getPathWithNumberAffixedFromAssetPath(char* tDest, const char* tSrc, int i);
void getFullPath(char* tDest, char* tPath);
void getPathToFile(char* tDest, char* tPath);

void printDirectory(char* tPath);


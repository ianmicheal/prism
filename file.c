#include "prism/file.h"

#include <sys/stat.h>

#include "prism/log.h"
#include "prism/memoryhandler.h"
#include "prism/system.h"

char* getPureFileName(char* path) {
	debugLog("Getting pure filename.");
	char* pos = strrchr(path, '/');

	if (pos == NULL) return path;
	else return pos + 1;
}


char* getFileExtension(char* tPath) {
	char* pos = strrchr(tPath, '.');

	if (pos == NULL) {
		logError("Unable to find file ending.");
		logErrorString(tPath);
		abortSystem();
	}

	return pos + 1;
}

void getPathWithoutFileExtension(char* tDest, char* tPath) {
	strcpy(tDest, tPath);
	if (!strcmp("", tPath)) return;

	char* pos = strrchr(tDest, '.');
	*pos = '\0';
}

void getPathWithNumberAffixedFromAssetPath(char* tDest, const char* tSrc, int i) {
	char name[1024];
	strcpy(name, tSrc);

	char* pos = strrchr(name, '.');
	if (pos == NULL) {
		sprintf(tDest, "%s%d", tSrc, i);
	}
	else {
		pos[0] = '\0';
		sprintf(tDest, "%s%d.%s", name, i, pos + 1);
		pos[0] = '.';
	}

}

void getPathToFile(char * tDest, char * tPath)
{
	strcpy(tDest, tPath);
	char* pathEnd = strrchr(tDest, '/');
	if (pathEnd != NULL) pathEnd[1] = '\0';
}

static int isFileMemoryMapped(FileHandler tFile) {
	void* data = fileMemoryMap(tFile);
	return data != NULL;
}

int isFile(char* tPath) {
	if(isDirectory(tPath)) return 0;

	FileHandler file = fileOpen(tPath, O_RDONLY);
	if (file == FILEHND_INVALID) return 0;
	fileClose(file);
	return 1;
}

int isDirectory(char* tPath) {
	struct stat sb;
	char path[1024];
	getFullPath(path, tPath);

	return (stat(path, &sb) == 0 && (sb.st_mode & S_IFDIR));
}


static Buffer makeBufferInternal(void * tData, uint32_t tLength, int tIsOwned)
{
	Buffer b;
	b.mData = tData;
	b.mLength = tLength;
	b.mIsOwned = tIsOwned;
	return b;
}

Buffer makeBuffer(void * tData, uint32_t tLength)
{
	return makeBufferInternal(tData, tLength, 0);
}

Buffer makeBufferOwned(void * tData, uint32_t tLength)
{
	return makeBufferInternal(tData, tLength, 1);
}

Buffer fileToBuffer(char* tFileDir) {
	debugLog("Reading file to Buffer.");
	Buffer ret;

	size_t bufferLength;
	FileHandler file;
	char* data;

	file = fileOpen(tFileDir, O_RDONLY);

	if (file == FILEHND_INVALID) {
		logError("Couldn't open file.");
		logErrorString(tFileDir);
		logErrorString(getWorkingDirectory());
		logErrorString(getFileSystem());
		abortSystem();
	}

	bufferLength = fileTotal(file);
	debugInteger(bufferLength);

	if (isFileMemoryMapped(file)) {
		data = fileMemoryMap(file);
		ret.mIsOwned = 0;
	}
	else {
		data = allocMemory(bufferLength);
		fileRead(file, data, bufferLength);
		ret.mIsOwned = 1;
	}

	ret.mData = data;
	ret.mLength = bufferLength;

	fileClose(file);

	return ret;
}

void bufferToFile(char* tFileDir, Buffer tBuffer) {
	FileHandler file = fileOpen(tFileDir, O_WRONLY);
	fileWrite(file, tBuffer.mData, tBuffer.mLength);
	fileClose(file);
}

void freeBuffer(Buffer buffer) {
	debugLog("Freeing buffer.");
	if (buffer.mIsOwned) {
		debugLog("Freeing owned memory");
		debugInteger(buffer.mLength);
		freeMemory(buffer.mData);
	}
	buffer.mData = NULL;
	buffer.mLength = 0;
	buffer.mIsOwned = 0;
}

void appendTerminationSymbolToBuffer(Buffer* tBuffer) {
	if (!tBuffer->mIsOwned) {
		char* nData = allocMemory(tBuffer->mLength + 1);
		memcpy(nData, tBuffer->mData, tBuffer->mLength);
		tBuffer->mData = nData;
		tBuffer->mIsOwned = 1;
	}
	else {
		tBuffer->mData = reallocMemory(tBuffer->mData, tBuffer->mLength + 1);
	}


	char* buf = tBuffer->mData;
	buf[tBuffer->mLength] = '\0';
	tBuffer->mLength++;
}

void fileToMemory(void* tDst, int tSize, char* tPath) {
	Buffer b = fileToBuffer(tPath);
	if (b.mLength != (unsigned int)tSize) {
		logError("File and memory struct have different sizes!");
		logErrorString(tPath);
		logErrorInteger(tSize);
		logErrorInteger(b.mLength);
		abortSystem();
	}
	memcpy(tDst, b.mData, tSize);

	freeBuffer(b);
}

BufferPointer getBufferPointer(Buffer tBuffer)
{
	return tBuffer.mData;
}

void readFromBufferPointer(void * tDst, BufferPointer* tPointer, uint32_t tSize)
{
	memcpy(tDst, *tPointer, tSize);
	(*tPointer) += tSize;
}

int readIntegerFromTextStreamBufferPointer(BufferPointer* tPointer) {
	int value;
	int size;
	int items = sscanf(*tPointer, "%d%n", &value, &size);
	if (items != 1) {
		logWarning("Unable to read integer value from stream.");
		value = 0;
	}
	(*tPointer) += size;
	return value;
}
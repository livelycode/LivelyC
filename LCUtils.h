
#ifndef LivelyStore_LCUtils_h
#define LivelyStore_LCUtils_h

#include <ftw.h>
#include <unistd.h>

#include "LCCore.h"
#include "LCString.h"
#include "LCArray.h"
#include "LCData.h"

void LCPrintf(LCObjectRef object);

char hexDigitToASCIChar(char hexDigit);
char asciCharToHexDigit(char hexDigit);
void byteToHexDigits(LCByte input, char* buffer);
LCByte hexDigitsToByte(char* hexDigits);
LCStringRef createHexString(LCByte data[], size_t length);
LCDataRef createDataFromHexString(LCStringRef hexString);
LCArrayRef createPathArray(LCStringRef path);
void writeToFile(LCByte data[], size_t length, char* filePath);
LCDataRef readFromFile(FILE* fd);
int makeDirectory(char* path);
int deleteDirectory(char *path);
LCStringRef getHomeFolder();

#endif
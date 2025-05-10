#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <SdFat.h>
#include <sdios.h>

// SdFat instance — global access for the sketch
extern SdFat32 sd;

// Functions for file read/write
bool writeFile(const char* filepath, const char* data);
String readFile(const char* filepath);

#endif

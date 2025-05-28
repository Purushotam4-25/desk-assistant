#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <SdFat.h>
#include <sdios.h>


extern SdFat32 sd;

bool writeFile(const char* filepath, const char* data);
String readFile(const char* filepath);

#endif

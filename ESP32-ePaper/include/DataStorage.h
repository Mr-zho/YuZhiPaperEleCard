#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <vector>
#include <SD.h>

class SDStorage 
{
public:
    SDStorage();
    ~SDStorage() = default;
public:
    std::vector<std::vector<uint8_t>> SD_ReadFont(String path);
    bool SD_Init();
};
#endif
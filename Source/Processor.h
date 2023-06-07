#pragma once
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../Thirdparty/stb_image.h"
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../Thirdparty/stb_image_write.h"

#include <filesystem>

#include "ConstData.h"
#include "StdUtil.h"
#include "ThreadPool.h"

struct ImageBind
{
    std::string Path;
    int Comp;
};

class Processor
{
private:
    static std::unordered_map<std::string, std::unordered_map<MapType, std::string>> Files;
    static std::unordered_map<MapType, std::string>& Selected;
    static int OutputCount;
    static int InputCount;

public:

    static void LookForJobs();
    static bool HasMap(MapType Type);
    static void ProcessJobs();
};


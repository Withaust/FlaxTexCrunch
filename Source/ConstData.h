#pragma once
#include <unordered_map>
#include <string>
#include <algorithm>
#include <vector>
#include <filesystem>

#include "Logger.h"

enum class MapType
{
    Color = 0,
    Mask,
    Emissive,
    Metallicity,
    Roughness,
    AO,
    Normal
};

class ConstData
{
private:
    static std::vector<std::pair<std::string, MapType>> SortedBinds;
public:
    static std::string InputFolder;
    static std::string OutputFolder;
    static std::unordered_map<std::string, MapType> ExtensionBinds;
    static std::string MapTypeString(MapType Type);
    static void PrintHelp();
    static bool HandleArgs(int argc, char** argv);
};

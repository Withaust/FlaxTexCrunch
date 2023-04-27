#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Disable annoying warnings
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NODRAWTEXT
#define NOCTLMGR
#define NOFLATSBAPIS

// Include Windows headers
#include <Windows.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

enum MapType
{
    Color = 0,
    Mask,
    Emissive,
    Metallicity,
    Roughness,
    AO,
    Normal
};

std::unordered_map<std::string, MapType> ExtensionBinds;
std::unordered_map<MapType, std::string> Files;

int Width;
int Height;
int Comp;
int ColorWidth;
int ColorHeight;

std::string File;

std::string MapTypeString(MapType Type)
{
    switch (Type)
    {
    default:
    case MapType::Color:
        return "Color";
    case MapType::Mask:
        return "Mask";
    case MapType::Emissive:
        return "Emissive";
    case MapType::Metallicity:
        return "Metallicity";
    case MapType::Roughness:
        return "Roughness";
    case MapType::AO:
        return "AO";
    case MapType::Normal:
        return "Normal";
    }
}

std::size_t ReplaceAll(std::string& inout, std::string_view what, std::string_view with)
{
    std::size_t count{};
    for (std::string::size_type pos{};
        inout.npos != (pos = inout.find(what.data(), pos, what.length()));
        pos += with.length(), ++count)
        inout.replace(pos, what.length(), with.data(), with.length());
    return count;
}

inline bool EndsWith(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool cmp(std::pair<std::string, MapType>& a, std::pair<std::string, MapType>& b)
{
    return a.second < b.second;
}

int ProcessArgs()
{
    std::cout << "\nThis application allows you to combine multiple textures into one" << std::endl;
    std::cout << "for the use with Flax Engine's custom \"Texture Crunch\" material node." << std::endl;
    std::cout << "_colmet - Color map [RGB] + (Optional) Metalness map [A] " << std::endl;
    std::cout << "_emimas - Emissive map [RGB] + (Optional) Mask [A]" << std::endl;
    std::cout << "_nmarog - Normal map [RG] + AO [B] + (Optional) Roughness [A]\n" << std::endl;
    std::cout << "Current filetype bindings:" << std::endl;
    std::vector<std::pair<std::string, MapType>> SortedBinds;
    for (auto& it : ExtensionBinds) 
    {
        SortedBinds.push_back(it);
    }
    sort(SortedBinds.begin(), SortedBinds.end(), cmp);
    for (const auto& Bind : SortedBinds)
    {
        std::cout << "Extension: \"" << Bind.first << "\" Type: " << MapTypeString(Bind.second) << std::endl;
    }
    return 0;
}

void BindExtensions()
{
#define MAPTYPE(Ext, Map) ExtensionBinds[Ext] = MapType::Map
    MAPTYPE("_t", Mask);
    MAPTYPE("_tr", Mask);
    MAPTYPE("_tran", Mask);
    MAPTYPE("_trans", Mask);
    MAPTYPE("transparency", Mask);
    MAPTYPE("alpha", Mask);
    MAPTYPE("mask", Mask);
    MAPTYPE("_e", Emissive);
    MAPTYPE("_em", Emissive);
    MAPTYPE("_emm", Emissive);
    MAPTYPE("emisive", Emissive);
    MAPTYPE("emmisive", Emissive);
    MAPTYPE("emissive", Emissive);
    MAPTYPE("emmmissive", Emissive);
    MAPTYPE("_m", Metallicity);
    MAPTYPE("_met", Metallicity);
    MAPTYPE("metalness", Metallicity);
    MAPTYPE("metalic", Metallicity);
    MAPTYPE("metallic", Metallicity);
    MAPTYPE("_r", Roughness);
    MAPTYPE("roughness", Roughness);
    MAPTYPE("_rgh", Roughness);
    MAPTYPE("_a", AO);
    MAPTYPE("_ao", AO);
    MAPTYPE("ambient", AO);
    MAPTYPE("occlusion", AO);
    MAPTYPE("_n", Normal);
    MAPTYPE("nrm", Normal);
    MAPTYPE("nm", Normal);
    MAPTYPE("norm", Normal);
    MAPTYPE("normal", Normal);
    MAPTYPE("normals", Normal);
#undef MAPTYPE
}

void LookupFiles()
{
    for (const auto& Entry : fs::directory_iterator(fs::current_path()))
    {
        if (Entry.path().extension().string() != ".png")
        {
            continue;
        }

        for (const auto& Bind : ExtensionBinds)
        {
            MapType TargetType = MapType::Color;

            if (EndsWith(Entry.path().string(), "_colmet") ||
                EndsWith(Entry.path().string(), "_emimas") ||
                EndsWith(Entry.path().string(), "_nmarog"))
            {
                break;
            }

            if (EndsWith(Entry.path().stem().string(), Bind.first))
            {
                TargetType = Bind.second;
            }

            if (Files.find(TargetType) != Files.end())
            {
                continue;
            }

            Files[TargetType] = Entry.path().string();
        }
    }
}

bool HasMap(MapType Type)
{
    return Files.find(Type) != Files.end();
}

int ErrorClose(std::string Error)
{
    HWND hWnd = GetForegroundWindow();
    MessageBoxA(hWnd, Error.c_str(), "Error", MB_OK);
    return 1;
}

#define START_PROCESSING(Map, CheckSize) \
    stbi_uc* Map##Texture = stbi_load(Files[MapType::Map].c_str(), &Width, &Height, &Comp, 0); \
    if (CheckSize && (Width != ColorWidth || Height != ColorHeight)) \
    { \
        return ErrorClose(std::string(#Map) + std::string(" texture must be the same size as the Color texture!")); \
    } \
    int Map##ByteOffset = 0; \
    std::cout << "Crunching " << Files[MapType::Map] << std::endl; \
    for (int i = 0; i < Width * Height * Comp; i += Comp)

#define STOP_PROCESSING(Map) \
    free(Map##Texture);

#define START_BLOCK(Block, Prefix) \
std::string Block##Path = File + Prefix; \
if (fs::exists(Block##Path)) \
{ \
    fs::remove(Block##Path); \
} \
unsigned char* Block##Bytes = new unsigned char[Width * Height * 4]; \
std::memset(Block##Bytes, 0, Width * Height * 4); \
bool Wrote##Block = false

#define STOP_BLOCK(Block) \
if (Wrote##Block) \
{ \
    stbi_write_png(Block##Path.c_str(), Width, Height, 4, Block##Bytes, Width * 4); \
    std::cout << "Finished crunching " << Block##Path << std::endl; \
} \
delete[] Block##Bytes

int ProcessCM()
{
    std::string CMPath = File + "_colmet.png";
    if (fs::exists(CMPath))
    {
        fs::remove(CMPath);
    }

    stbi_uc* ColorTexture = stbi_load(Files[MapType::Color].c_str(), &Width, &Height, &Comp, 0);

    unsigned char* CMBytes = new unsigned char[Width * Height * 4];
    std::memset(CMBytes, 0, Width * Height * 4);
    bool WroteCM = true;

    if (Width != Height)
    {
        return ErrorClose("Color texture must have same width and height!");
    }

    ColorWidth = Width;
    ColorHeight = Height;

    int ColorByteOffset = 0;
    std::cout << "Crunching " << Files[MapType::Color] << std::endl;
    for (int i = 0; i < Width * Height * Comp; i += Comp)
    {
        CMBytes[ColorByteOffset] = ColorTexture[i];
        CMBytes[ColorByteOffset + 1] = ColorTexture[i + 1];
        CMBytes[ColorByteOffset + 2] = ColorTexture[i + 2];
        ColorByteOffset += 4;
    }

    free(ColorTexture);

    if (HasMap(MapType::Metallicity))
    {
        START_PROCESSING(Metallicity, true)
        {
            CMBytes[MetallicityByteOffset + 3] = MetallicityTexture[i];
            MetallicityByteOffset += 4;
        }
        STOP_PROCESSING(Metallicity)
    }

    STOP_BLOCK(CM);

    return 0;
}

int ProcessEM()
{
    START_BLOCK(EM, "_emimas.png");

    if (HasMap(MapType::Emissive))
    {
        WroteEM = true;
        START_PROCESSING(Emissive, true)
        {
            EMBytes[EmissiveByteOffset] = EmissiveTexture[i];
            EMBytes[EmissiveByteOffset + 1] = EmissiveTexture[i + 1];
            EMBytes[EmissiveByteOffset + 2] = EmissiveTexture[i + 2];
            EmissiveByteOffset += 4;
        }
        STOP_PROCESSING(Emissive)
    }

    if (HasMap(MapType::Mask))
    {
        WroteEM = true;
        START_PROCESSING(Mask, true)
        {
            EMBytes[MaskByteOffset + 3] = MaskTexture[i];
            MaskByteOffset += 4;
        }
        STOP_PROCESSING(Mask)
    }

    STOP_BLOCK(EM);

    return 0;
}

int ProcessNAR()
{
    START_BLOCK(NAR, "_nmarog.png");

    if (HasMap(MapType::Normal))
    {
        WroteNAR = true;
        START_PROCESSING(Normal, true)
        {
            NARBytes[NormalByteOffset] = NormalTexture[i];
            NARBytes[NormalByteOffset + 1] = NormalTexture[i + 1];
            NormalByteOffset += 4;
        }
        STOP_PROCESSING(Normal)
    }

    if (HasMap(MapType::AO))
    {
        WroteNAR = true;
        START_PROCESSING(AO, true)
        {
            NARBytes[AOByteOffset + 2] = AOTexture[i];
            AOByteOffset += 4;
        }
        STOP_PROCESSING(AO)
    }

    if (HasMap(MapType::Roughness))
    {
        WroteNAR = true;
        START_PROCESSING(Roughness, true)
        {
            NARBytes[RoughnessByteOffset + 3] = RoughnessTexture[i];
            RoughnessByteOffset += 4;
        }
        STOP_PROCESSING(Roughness)
    }

    STOP_BLOCK(NAR);

    return 0;
}

#undef START_PROCESSING
#undef STOP_PROCESSING
#undef START_BLOCK
#undef STOP_BLOCK

int main(int argc, char** argv)
{
    BindExtensions();
    LookupFiles();

    if (argc > 1)
    {
        return ProcessArgs();
    }

    if (!HasMap(MapType::Color))
    {
        return ErrorClose("Color texture must be present!");
    }

    if (HasMap(MapType::Mask) && !HasMap(MapType::Emissive))
    {
        return ErrorClose("Mask texture cant work without Emmisive texture!");
    }

    if (HasMap(MapType::Normal) && !HasMap(MapType::AO))
    {
        return ErrorClose("Normal texture cant work without AO texture!");
    }

    if (!HasMap(MapType::Normal) && HasMap(MapType::AO))
    {
        return ErrorClose("AO texture cant work without Normal texture!");
    }

    File = Files[MapType::Color];
    ReplaceAll(File, ".png", "");

    if (ProcessCM() != 0)
    {
        return 1;
    }

    if (ProcessEM() != 0)
    {
        return 1;
    }

    if (ProcessNAR() != 0)
    {
        return 1;
    }

    return 0;
}

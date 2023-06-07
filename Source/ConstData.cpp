#include "ConstData.h"

std::string ConstData::InputFolder;
std::string ConstData::OutputFolder;
std::vector<std::pair<std::string, MapType>> ConstData::SortedBinds;
std::unordered_map<std::string, MapType> ConstData::ExtensionBinds{
#define MAPTYPE(Ext, Map) { std::string(Ext) + std::string(".png"), MapType::Map }
MAPTYPE("_t", Mask),
MAPTYPE("_tr", Mask),
MAPTYPE("_tran", Mask),
MAPTYPE("_trans", Mask),
MAPTYPE("_transparency", Mask),
MAPTYPE("_alpha", Mask),
MAPTYPE("_mask", Mask),
MAPTYPE("_e", Emissive),
MAPTYPE("_em", Emissive),
MAPTYPE("_emm", Emissive),
MAPTYPE("_emisive", Emissive),
MAPTYPE("_emmisive", Emissive),
MAPTYPE("_emissive", Emissive),
MAPTYPE("_emmmissive", Emissive),
MAPTYPE("_m", Metallicity),
MAPTYPE("_met", Metallicity),
MAPTYPE("_metalness", Metallicity),
MAPTYPE("_metalic", Metallicity),
MAPTYPE("_metallic", Metallicity),
MAPTYPE("_r", Roughness),
MAPTYPE("_roughness", Roughness),
MAPTYPE("_rgh", Roughness),
MAPTYPE("_a", AO),
MAPTYPE("_ao", AO),
MAPTYPE("_ambient", AO),
MAPTYPE("_occlusion", AO),
MAPTYPE("_n", Normal),
MAPTYPE("_nrm", Normal),
MAPTYPE("_nm", Normal),
MAPTYPE("_norm", Normal),
MAPTYPE("_normal", Normal),
MAPTYPE("_normals", Normal)
#undef MAPTYPE
};

std::string ConstData::MapTypeString(MapType Type)
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

void ConstData::PrintHelp()
{
	Logger::Print("FlaxTexCrunch.exe InputFolder OutputFolder");
	Logger::Print("\n\nThis application allows you to combine multiple textures into one");
	Logger::Print("for the use with Unary's custom \"TextureUncrunch\" material node for Flax.");
	Logger::Print("_colm - Color map [RGB] + (Optional) Metalness map [A] ");
	Logger::Print("_emim - Emissive map [RGB] + (Optional) Mask [A]");
	Logger::Print("_nmaor - Normal map [RG] + AO [B] + (Optional) Roughness [A]\n");
	Logger::Print("Current filetype bindings:");

	// Bindings were never sorted before, sort
	if (SortedBinds.size() == 0)
	{
		for (auto& it : ExtensionBinds)
		{
			SortedBinds.push_back(it);
		}
		std::sort(SortedBinds.begin(), SortedBinds.end(), [](const std::pair<std::string, MapType>& a, const std::pair<std::string, MapType>& b) -> bool
			{
				return a.second < b.second;
			});
	}

	// Print bindings
	for (const auto& Bind : SortedBinds)
	{
		Logger::Print("Extension: \"" + Bind.first + "\" Type: " + MapTypeString(Bind.second));
	}
}

bool ConstData::HandleArgs(int argc, char** argv)
{
	if (argc != 3)
	{
		PrintHelp();
		return false;
	}
	InputFolder = argv[1];
	OutputFolder = argv[2];
	
	if (!std::filesystem::exists(ConstData::InputFolder))
	{
		Logger::Error("Input folder: \"" + ConstData::InputFolder + "\" could not be found.");
		return false;
	}

	Logger::Info("Input folder: \"" + InputFolder + "\" Output folder: \"" + OutputFolder + "\"");

	return true;
}

#include "Processor.h"

std::unordered_map<std::string, std::unordered_map<MapType, std::string>> Processor::Files;
std::unordered_map<MapType, std::string> NonSelected;
std::unordered_map<MapType, std::string>& Processor::Selected = NonSelected;
int Processor::OutputCount = 0;
int Processor::InputCount = 0;

void Processor::LookForJobs()
{
	if (std::filesystem::exists(ConstData::OutputFolder))
	{
		std::filesystem::remove_all(ConstData::OutputFolder);
	}

	for (std::filesystem::recursive_directory_iterator i(ConstData::InputFolder), end; i != end; ++i)
	{
		if (std::filesystem::is_directory(i->path()))
		{
			continue;
		}

		std::string Path = i->path().string();
		StdUtil::ReplaceAll(Path, "\\", "/");

		if (!StdUtil::EndsWith(Path, ".png"))
		{
			continue;
		}

		MapType TargetType = MapType::Color;
		std::string TargetBind = ".png";

		for (const auto& Bind : ConstData::ExtensionBinds)
		{
			if (StdUtil::EndsWith(Path, Bind.first))
			{
				TargetType = Bind.second;
				TargetBind = Bind.first;
				break;
			}
		}

		std::string OutputPath = Path;
		StdUtil::ReplaceAll(Path, ConstData::InputFolder + "/", ConstData::OutputFolder + "/");
		StdUtil::ReplaceAll(Path, TargetBind, "");

		Files[Path][TargetType] = OutputPath;
		InputCount++;
	}
}

bool Processor::HasMap(MapType Type)
{
	return Selected.find(Type) != Selected.end();
}

bool Process(std::vector<ImageBind> Images, std::string Output)
{
	int ImageSize = 0;
	unsigned char* OutputBuffer = nullptr;
	int FirstWidth = 0;
	int FirstHeight = 0;
	std::string FirstPath;
	int Width = 0;
	int Height = 0;
	int Comp = 0;
	int CompOffset = 0;
	int CompIncrement = 0;

	// Firstly we assign all the textures to their respective texture slots
	// while doing some sanity checks
	for (size_t i = 0; i < Images.size(); ++i)
	{
		const auto& Image = Images[i];

		stbi_uc* Texture;
		if (FirstWidth == 0 && FirstHeight == 0)
		{
			FirstPath = Image.Path;
			Texture = stbi_load(Image.Path.c_str(), &Width, &Height, &Comp, 0);
			FirstWidth = Width;
			FirstHeight = Height;
			ImageSize = Width * Height * 4;
			OutputBuffer = new unsigned char[ImageSize];
			std::memset(OutputBuffer, 0, ImageSize);
		}
		else
		{
			Texture = stbi_load(Image.Path.c_str(), &Width, &Height, &Comp, 0);
		}

		if (Width != Height)
		{
			Logger::Error(Image.Path + " does not have a square ratio, abandoning " + Output);
			stbi_image_free(Texture);
			delete[] OutputBuffer;
			return false;
		}

		if (Width != FirstWidth || Height != FirstHeight)
		{
			Logger::Error(Image.Path + " does not have the same resolution as " + FirstPath + ", abandoning " + Output);
			stbi_image_free(Texture);
			delete[] OutputBuffer;
			return false;
		}

		if (Comp < Image.Comp)
		{
			Logger::Error(Image.Path + " does not have a minimum required comp (expected " + std::to_string(Image.Comp) + " got " + std::to_string(Comp) + "), abandoning " + Output);
			stbi_image_free(Texture);
			delete[] OutputBuffer;
			return false;
		}

		CompIncrement = 0;

		for (int j = 0; j < Width * Height * Comp; j += Comp)
		{
			std::memcpy(OutputBuffer + CompIncrement + CompOffset, Texture + j, Image.Comp);
			CompIncrement += 4;
		}

		CompOffset += Image.Comp;
		stbi_image_free(Texture);
	}

	stbi_write_png(Output.c_str(), Width, Height, 4, OutputBuffer, Width * 4);
	delete[] OutputBuffer;

	return true;
}

void Processor::ProcessJobs()
{
	auto Start = std::chrono::high_resolution_clock::now();

	ThreadPool ThreadPool;

	for (const auto& Entry : Files)
	{
		Selected = Entry.second;

		if (!HasMap(MapType::Color))
		{
			Logger::Error(Entry.first + " does not have a Color map, skipping...");
			continue;
		}

		// Create output directories
		std::filesystem::path Path = Entry.first;
		std::filesystem::create_directories(Path.parent_path());

		std::vector<ImageBind> Binds;
		Binds.push_back({ Entry.second.at(MapType::Color), 3 });
		if (HasMap(MapType::Metallicity))
		{
			Binds.push_back({ Entry.second.at(MapType::Metallicity), 1 });
		}

		ThreadPool.AddTask(Process, Binds, Entry.first + "_colm.png");
		OutputCount++;
		Binds.clear();

		if (HasMap(MapType::Mask) && !HasMap(MapType::Emissive))
		{
			Logger::Error(Entry.first + " has mask texture without emmisive texture, skipping...");
			continue;
		}

		if (HasMap(MapType::Emissive))
		{
			Binds.push_back({ Entry.second.at(MapType::Emissive), 3 });
			if (HasMap(MapType::Mask))
			{
				Binds.push_back({ Entry.second.at(MapType::Mask), 1 });
			}

			ThreadPool.AddTask(Process, Binds, Entry.first + "_emim.png");
			OutputCount++;
			Binds.clear();
		}

		if (HasMap(MapType::Normal) && !HasMap(MapType::AO))
		{
			Logger::Error(Entry.first + " has normal texture without AO texture, skipping...");
			continue;
		}

		if (!HasMap(MapType::Normal) && HasMap(MapType::AO))
		{
			Logger::Error(Entry.first + " has AO texture without normal texture, skipping...");
			continue;
		}

		if (HasMap(MapType::Normal) && HasMap(MapType::AO))
		{
			Binds.push_back({ Entry.second.at(MapType::Normal), 2 });
			Binds.push_back({ Entry.second.at(MapType::AO), 1 });
			if (HasMap(MapType::Roughness))
			{
				Binds.push_back({ Entry.second.at(MapType::Roughness), 1 });
			}

			ThreadPool.AddTask(Process, Binds, Entry.first + "_nmaor.png");
			OutputCount++;
			Binds.clear();
		}
	}

	ThreadPool.WaitAll();

	auto Stop = std::chrono::high_resolution_clock::now();
	Logger::Info("Finished crunching " + std::to_string(InputCount) + " textures to " + std::to_string(OutputCount) + " textures");
	Logger::Info("Took us: " + std::to_string(duration_cast<std::chrono::seconds>(Stop - Start).count()) + "s");
}

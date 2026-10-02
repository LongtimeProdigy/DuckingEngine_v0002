#pragma once

#include <string>
#include <vector>
#include "lib/tinyXML/tinyxml.h"

namespace DK
{
	// Index zero keeps the original name. The last XML dimension varies fastest.
	inline std::string pipelinePermutationName(const std::string& name, unsigned int index)
	{
		return index == 0 ? name : name + "[" + std::to_string(index) + "]";
	}

	inline bool parsePipelinePermutations(const TiXmlElement& pipeline, std::vector<std::vector<std::string>>& combinations, std::string& error)
	{
		constexpr size_t maxCombinations = 4096;
		combinations = { {} };
		error.clear();
		std::vector<std::string> names;
		for (const TiXmlElement* node = pipeline.FirstChildElement("Permutation"); node; node = node->NextSiblingElement("Permutation"))
		{
			const char* nameAttribute = node->Attribute("Name");
			const char* valuesAttribute = node->Attribute("Values");
			if (!nameAttribute || !*nameAttribute || !valuesAttribute || !*valuesAttribute)
			{
				error = "Permutation requires non-empty Name and Values attributes.";
				return false;
			}
			const std::string name = nameAttribute;
			for (size_t i = 0; i < name.size(); ++i)
			{
				const char c = name[i];
				if ((c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (i > 0 && c >= '0' && c <= '9')) == false)
				{
					error = "Invalid permutation macro name: " + name;
					return false;
				}
			}
			for (const std::string& previous : names)
			{
				if (previous == name)
				{
					error = "Duplicate permutation name: " + name;
					return false;
				}
			}
			names.push_back(name);
			std::vector<std::string> values;
			const std::string list = valuesAttribute;
			size_t begin = 0;
			do
			{
				const size_t end = list.find(',', begin);
				std::string value = list.substr(begin, end == std::string::npos ? end : end - begin);
				const size_t first = value.find_first_not_of(" \t\r\n");
				if (first == std::string::npos)
				{
					error = "Empty permutation value: " + name;
					return false;
				}
				value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
				for (const std::string& previous : values)
				{
					if (previous == value)
					{
						error = "Duplicate permutation value: " + name + "=" + value;
						return false;
					}
				}
				values.push_back(value);
				if (values.size() > maxCombinations / combinations.size())
				{
					error = "A pipeline may have at most 4096 permutation combinations.";
					return false;
				}
				if (end == std::string::npos)
					break;
				begin = end + 1;
			} while (true);

			std::vector<std::vector<std::string>> expanded;
			expanded.reserve(combinations.size() * values.size());
			for (const std::vector<std::string>& combination : combinations)
			{
				for (const std::string& value : values)
				{
					expanded.push_back(combination);
					expanded.back().push_back(name + "=" + value);
				}
			}
			combinations.swap(expanded);
		}
		return true;
	}
}

#pragma once

#include "CommonTypes.h"
#include "StringUtil.h"

#include "lib/tinyXML/tinyxml.h"

namespace DK
{
	// Index zero keeps the original name. The last XML dimension varies fastest.
	dk_inline DKString pipelinePermutationName(const DKString& name, unsigned int index)
	{
		return index == 0 ? name : name + "[" + StringUtil::toString(index) + "]";
	}

	dk_inline bool parsePipelinePermutations(const TiXmlElement& pipeline, DKVector<DKVector<DKString>>& combinations)
	{
		constexpr size_t maxCombinations = 4096;

		combinations = { {} };
		DKVector<DKString> names;
		for (const TiXmlElement* node = pipeline.FirstChildElement("Permutation"); node; node = node->NextSiblingElement("Permutation"))
		{
			const char* nameAttribute = node->Attribute("Name");
			const char* valuesAttribute = node->Attribute("Values");
			if (nameAttribute == nullptr || *nameAttribute == '\0' || valuesAttribute == nullptr || *valuesAttribute == '\0')
			{
				DK_ASSERT_LOG(false, "Permutation requires non-empty Name and Values attributes.");
				return false;
			}

			const DKString name = nameAttribute;
			for (size_t i = 0; i < name.size(); ++i)
			{
				const char c = name[i];
				if ((c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (i > 0 && c >= '0' && c <= '9')) == false)
				{
					DK_ASSERT_LOG(false, "Invalid permutation macro name: %s", name.c_str());
					return false;
				}
			}
			for (const DKString& previous : names)
			{
				if (previous == name)
				{
					DK_ASSERT_LOG(false, "Duplicate permutation name: %s", name.c_str());
					return false;
				}
			}
			names.push_back(name);

			DKVector<DKString> values;
			const DKString list = valuesAttribute;
			size_t begin = 0;
			do
			{
				const size_t end = list.find(',', begin);
				DKString value = list.substr(begin, end == DKString::npos ? end : end - begin);
				const size_t first = value.find_first_not_of(" \t\r\n");
				if (first == DKString::npos)
				{
					DK_ASSERT_LOG(false, "Empty permutation value: %s", name.c_str());
					return false;
				}
				value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
				for (const DKString& previous : values)
				{
					if (previous == value)
					{
						DK_ASSERT_LOG(false, "Duplicate permutation value: %s = %s", name.c_str(), value.c_str());
						return false;
					}
				}
				values.push_back(value);
				if (values.size() > maxCombinations / combinations.size())
				{
					DK_ASSERT_LOG(false, "A pipeline may have at most 4096 permutation combinations.");
					return false;
				}

				if (end == DKString::npos)
					break;

				begin = end + 1;
			} while (true);

			DKVector<DKVector<DKString>> expanded;
			expanded.reserve(combinations.size() * values.size());
			for (const DKVector<DKString>& combination : combinations)
			{
				for (const DKString& value : values)
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

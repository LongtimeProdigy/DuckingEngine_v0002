#include "stdafx.h"
#include "Animation.h"

namespace DK
{
	DKVector<std::string> split(const std::string& str, const std::string& delim)
	{
		DKVector<std::string> tokens;
		size_t prev = 0, pos = 0;
		do
		{
			pos = str.find(delim, prev);
			if (pos == std::string::npos) pos = str.length();
			std::string token = str.substr(prev, pos - prev);
			if (!token.empty()) tokens.push_back(token);
			prev = pos + delim.length();
		} while (pos < str.length() && prev < str.length());
		return tokens;
	}
	float3 parseFloat3FromString(const std::string& str)
	{
		DKVector<std::string> tokens = split(str, " ");
		return float3(
			StringUtil::atof(tokens[0].c_str()),
			StringUtil::atof(tokens[1].c_str()),
			StringUtil::atof(tokens[2].c_str())
		);
	}
	Quaternion parseQuaternionFromString(const std::string& str)
	{
		DKVector<std::string> tokens = split(str, " ");
		return Quaternion(
			StringUtil::atof(tokens[0].c_str()),
			StringUtil::atof(tokens[1].c_str()),
			StringUtil::atof(tokens[2].c_str()),
			StringUtil::atof(tokens[3].c_str())
		);
	}

	const bool Animation::loadAnimatoin(const DKString& animationPath)
	{
		ScopeString<DK_MAX_PATH> animationFullPath = GlobalPath::makeResourceFullPath(animationPath);

		TiXmlDocument doc;
		doc.LoadFile(animationFullPath.c_str());
		TiXmlElement* rootNode = doc.FirstChildElement("Animation");
		if (rootNode == nullptr)
		{
			DK_ASSERT_LOG(false, "Animation XML의 RootNode가 Animation이 아닙니다.");
			return false;
		}

		const char* boneCountArr = rootNode->Attribute("BoneCount");
		const char* frameCountAttr = rootNode->Attribute("FrameCount");
		if (boneCountArr == nullptr || frameCountAttr == nullptr)
		{
			DK_ASSERT_LOG(false, "Animation Data의 FrameCount Attribute를 찾을 수 없습니다.");
			return false;
		}

		const uint32 boneCount = atoi(boneCountArr);
		if (boneCount == 0)
			return false;
		const uint32 frameCount = atoi(frameCountAttr);
		if (frameCount == 0)
			return false;

		DKVector<Animation::BoneAnimation> boneAnimations;
		boneAnimations.resize(boneCount);

		uint32 boneIndex = 0;
		for (TiXmlNode* boneNode = rootNode->FirstChild(); boneNode != nullptr; boneNode = boneNode->NextSibling())
		{
			DKString boneName = boneNode->ToElement()->FirstAttribute()->Value();

			Animation::BoneAnimation& boneAnimation = boneAnimations[boneIndex];
#if defined(_DK_DEBUG_)
			boneAnimation._boneName = boneName;
#endif
			boneAnimation._animation.resize(frameCount);

			uint32 boneFrameCount = 0;
			for (TiXmlNode* transformNode = boneNode->FirstChild(); transformNode != nullptr; transformNode = transformNode->NextSibling())
			{
				TiXmlNode* positionNode = transformNode->FirstChild("Position");
				TiXmlNode* rotationNode = transformNode->FirstChild("Rotation");
				TiXmlNode* scaleNode = transformNode->FirstChild("Scale");
				if (positionNode == nullptr || rotationNode == nullptr || scaleNode == nullptr)
				{
					DK_ASSERT_LOG(false, "Transform노드를 Decompose할 수 없습니다.");
					return false;
				}

				DKString positionStr = positionNode->FirstChild()->Value();
				DKString rotationStr = rotationNode->FirstChild()->Value();
				DKString scaleStr = scaleNode->FirstChild()->Value();

				float3 position = parseFloat3FromString(positionStr);
				Quaternion rotation = parseQuaternionFromString(rotationStr);
				float3 scale = parseFloat3FromString(scaleStr);

				if (boneFrameCount >= frameCount)
				{
					DK_ASSERT_LOG(false, "FrameCount와 BoneFrameCount가 다릅니다. %d / %d", boneFrameCount, frameCount);
					return false;
				}

				Transform animation(position, rotation, scale);
				animation.tofloat4x4(boneAnimation._animation[boneFrameCount]);

				++boneFrameCount;
			}

			if (frameCount != boneFrameCount)
			{
				DK_ASSERT_LOG(false, "FrameCount와 BoneFrameCount가 다릅니다. %d / %d", boneFrameCount, frameCount);
				return false;
			}
		}

		_frameCount = frameCount;
		_boneAnimations.swap(boneAnimations);

		return true;
	}
}

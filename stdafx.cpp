#include "stdafx.h"

namespace DK
{

	float3 StringUtil::parseFloat3FromString(const DKString& str)
	{
		const DKVector<DKString> tokens = split(str, " ");
		DK_ASSERT_LOG(tokens.size() >= 3, "A float3 requires three components.");
		return float3(atof(tokens[0].c_str()), atof(tokens[1].c_str()), atof(tokens[2].c_str()));
	}

	Quaternion StringUtil::parseQuaternionFromString(const DKString& str)
	{
		const DKVector<DKString> tokens = split(str, " ");
		DK_ASSERT_LOG(tokens.size() >= 4, "A quaternion requires four components.");
		return Quaternion(atof(tokens[0].c_str()), atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
	}

	DKString GlobalPath::kResourcePathA;
	DKStringW GlobalPath::kResourcePathW;

#if defined(_DK_DEBUG_)
	ThreadID gMainThreadID = 0;
#endif
}

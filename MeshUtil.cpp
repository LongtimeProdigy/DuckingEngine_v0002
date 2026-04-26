#include "stdafx.h"
#include "MeshUtil.h"

namespace DK
{
	namespace MeshUtil
	{
		const bool createSphere(const uint32 tessellationX, const uint32 tessellationY, const float radius, DKVector<float3>& positionArr, DKVector<float3>& normalArr, DKVector<float2>& uvArr, DKVector<uint32>& indexArr)
		{
			const float degreeY = (180.0f / tessellationY) * DK::Math::kToRadian;
			const float degreeX = (360.0f / tessellationX) * DK::Math::kToRadian;

			// Vertex
			positionArr.resize((tessellationY - 1) * tessellationX + 2);	// 맨 위, 맨 아래 점 2개는 따로 +로 추가s
			uint32 n = 0;
			positionArr[n++] = float3(0, radius, 0);
			for (uint32 i = 1; i < tessellationY; ++i)
			{
				float positionY = radius * DK::Math::sin(DK::Math::Half_PI - degreeY * i);
				float radiusXZ = radius * DK::Math::cos(DK::Math::Half_PI - degreeY * i);
				for (uint32 j = 0; j < tessellationX; ++j)
				{
					float positionX = radiusXZ * DK::Math::cos(degreeX * j);
					float positionZ = radiusXZ * DK::Math::sin(degreeX * j);

					positionArr[n++] = float3(positionX, positionY, positionZ);
				}
			}
			positionArr[n++] = float3(0, -radius, 0);
			DK_ASSERT_LOG(n == positionArr.size(), "Sphere VertexCount가 올바르지 않음");

			// Index
			// 맨위 삼각형 + 중간라인 + 맨 마지막 점
			const uint32 indexCount = (tessellationX * 3) + (tessellationX * 6) * (tessellationY - 2) + (tessellationX * 3);
			indexArr.resize(indexCount);
			n = 0;
			for (uint32 i = 0; i < tessellationY; ++i)
			{
				for (uint32 j = 0; j < tessellationX; ++j)
				{
					uint32 innerCircleVetexIndex0 = i == 0 ? 0 : 1 + (i - 1) * tessellationX + j;
					uint32 innerCircleVetexIndex1 = j == tessellationX - 1 ? 1 + (i - 1) * tessellationX : 1 + (i - 1) * tessellationX + j + 1;
					uint32 outerCircleVertexIndex0 = 1 + i * tessellationX + j;
					uint32 outerCircleVertexIndex1 = i == tessellationY - 1 ? positionArr.size() - 1 : j == tessellationX - 1 ? 1 + i * tessellationX : 1 + i * tessellationX + j + 1;

					if (i != tessellationY - 1)
					{
						indexArr[n++] = innerCircleVetexIndex0;
						indexArr[n++] = outerCircleVertexIndex0;
						indexArr[n++] = outerCircleVertexIndex1;
					}

					if (i != 0)
					{
						indexArr[n++] = innerCircleVetexIndex0;
						indexArr[n++] = outerCircleVertexIndex1;
						indexArr[n++] = innerCircleVetexIndex1;
					}
				}
			}
			DK_ASSERT_LOG(n == indexArr.size(), "Sphere IndexCount가 올바르지 않음");

			return true;
		}

		const bool createPlane(const float width, const float depth, const uint32 m, const uint32 n, DKVector<float3>& positionArr, DKVector<float3>& normalArr, DKVector<float2>& uvArr, DKVector<uint32>& indexArr)
		{
			const uint32 vertexCount = m * n;
            const uint32 faceCount = (m - 1) * (n - 1) * 2;
            
            // 정점 위치 계산
            const float halfWidth = 0.5f * width;
            const float halfDepth = 0.5f * depth;
            
            const float dx = static_cast<float>(width) / (n - 1);
            const float dz = static_cast<float>(depth) / (m - 1);
            
            const float du = 1.0f / (n - 1);
            const float dv = 1.0f / (m - 1);
            
            positionArr.resize(vertexCount);
            normalArr.resize(vertexCount);
            uvArr.resize(vertexCount);
            for (uint32_t i = 0; i < m; ++i)
            {
                float z = halfDepth - i * dz;
                for (uint32_t j = 0; j < n; ++j)
                {
                    float x = -halfWidth + j * dx;
            
                    positionArr[i * n + j] = float3(x, 0.0f, z);
                    normalArr[i * n + j] = float3(0.0f, 1.0f, 0.0f); // 초기 노멀은 위쪽 방향
                    uvArr[i * n + j] = float2(j * du, i * dv);
                }
            }
            
            // 인덱스 계산
            indexArr.resize(faceCount * 3); // 삼각형 하나당 3개의 인덱스
            uint32_t k = 0;
            for (uint32_t i = 0; i < m - 1; ++i)
            {
                for (uint32_t j = 0; j < n - 1; ++j)
                {
                    // DirectX는 기본적으로 시계 방향(CW)을 컬링하지 않으므로, 인덱스를 CW 순서로 추가합니다.
                    indexArr[k] = i * n + j;
                    indexArr[k + 1] = i * n + j + 1;
                    indexArr[k + 2] = (i + 1) * n + j;
            
                    indexArr[k + 3] = (i + 1) * n + j;
                    indexArr[k + 4] = i * n + j + 1;
                    indexArr[k + 5] = (i + 1) * n + j + 1;
            
                    k += 6; // 2개의 삼각형(사각형 하나) 추가
                }
            }

			return true;
		}
	}
}
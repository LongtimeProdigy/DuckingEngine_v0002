#include "stdafx.h"
#include "RenderModule.h"

#include "RenderModuleDX12.h"

uint RenderModule::kFrameCount = 3;

RenderModule::RenderModule()
{

}
RenderModule::~RenderModule()
{

}

RenderModule* RenderModule::CreateRenderModule(RenderModuleType type, uint frameCount)
{
	RenderModule* renderModule = nullptr;
	switch (type)
	{
	case RenderModuleType::DIRECTX12:
		renderModule = dk_new RenderModuleDX12;
	break;
	case RenderModuleType::DIRECTX11:
	case RenderModuleType::VULKAN:
	case RenderModuleType::OPENGL:
	default:
		DK_ASSERT_LOG(false, "현재 지원하지 않는 RenderType입니다.");
	break;
	}

	renderModule->_isRunning = true;
	return renderModule;
}
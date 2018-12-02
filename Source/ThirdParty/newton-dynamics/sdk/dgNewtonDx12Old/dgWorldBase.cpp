/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgNewtonPluginStdafx.h"
#include "dgWorldBase.h"


using namespace DirectX;
using namespace Microsoft::WRL;
//using namespace Windows::Foundation;
//using namespace Windows::Graphics::Display;
//using namespace Windows::UI::Core;
//using namespace Windows::UI::Xaml::Controls;
//using namespace Platform;


// This is an example of an exported function.
dgWorldPlugin* GetPlugin(dgWorld* const world, dgMemoryAllocator* const allocator)
{
	
	static dgWorldBase module(world, allocator);
	module.m_score = 5;

	return &module;
}

dgWorldBase::dgWorldBase(dgWorld* const world, dgMemoryAllocator* const allocator)
	:dgWorldPlugin(world, allocator)
	,dgSolver(world, allocator)
{
	CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory));
	GetHardwareAdapter();

	//ComPtr<IDXGIAdapter1> adapter;
	//ComPtr<DX::ID3D11Device> device;
	//ComPtr<ID3D11DeviceContext> context;
}

dgWorldBase::~dgWorldBase()
{
}

const char* dgWorldBase::GetId() const
{
	return m_deviceName;
//	return "gpu experimental";
}

dgInt32 dgWorldBase::GetScore() const
{
	return m_score;
}


void dgWorldBase::GetHardwareAdapter()
{
	IDXGIAdapter1* adapter = nullptr;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter); adapterIndex++)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			for (int i = 0; desc.Description[i]; i++) {
				wctomb(&m_deviceName[i], desc.Description[i]);
				m_deviceName[i + 1] = 0;
			}
			#ifdef _DEBUG
			strcat(m_deviceName, "_d");
			#endif
			m_adapter = adapter;
			break;
		}
		adapter->Release();
	}
}


void dgWorldBase::CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, dgFloat32 timestep)
{
	DG_TRACKTIME_NAMED(GetId());
	dgSolver::CalculateJointForces(cluster, bodyArray, jointArray, timestep);
}

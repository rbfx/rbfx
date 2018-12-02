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

#ifndef _DG_WORLD_BASE_H_
#define _DG_WORLD_BASE_H_
#include "dgNewtonPluginStdafx.h"
#include "dgSolver.h"

#ifdef __cplusplus 
extern "C"
{
	NEWTONCPU_API dgWorldPlugin* GetPlugin(dgWorld* const world, dgMemoryAllocator* const allocator);
}
#endif


class dgWorldBase: public dgWorldPlugin, public dgSolver
{
	public:
	dgWorldBase(dgWorld* const world, dgMemoryAllocator* const allocator);
	virtual ~dgWorldBase();

	virtual const char* GetId() const;
	virtual dgInt32 GetScore() const;
	virtual void CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, dgFloat32 timestep);

	void GetHardwareAdapter();

	int m_score;
	char m_deviceName[64];

	// Direct3D objects.
	//Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
};

#endif
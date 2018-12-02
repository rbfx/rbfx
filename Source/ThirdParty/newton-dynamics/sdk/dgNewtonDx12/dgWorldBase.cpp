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


// This is an example of an exported function.
dgWorldPlugin* GetPlugin(dgWorld* const world, dgMemoryAllocator* const allocator)
{
	static dgWorldBase module(world, allocator);
	return &module;
}

dgWorldBase::dgWorldBase(dgWorld* const world, dgMemoryAllocator* const allocator)
	:dgWorldPlugin(world, allocator)
	,dgSolver(world, allocator)
	,m_score(5)
//	,m_accelerator(accelerator::default_accelerator)
//	,m_accelerator(accelerator::cpu_accelerator)
	,m_accelerator(accelerator::direct3d_warp)
//	,m_accelerator(accelerator::direct3d_ref)
{
	accelerator_view acc_v = m_accelerator.default_view;

	std::wstring desc (m_accelerator.get_description());
	for (int i = 0; i < dgInt32 (desc.size()); i++) {
		wctomb(&m_deviceName[i], desc[i]);
		m_deviceName[i + 1] = 0;
	}
#ifdef _DEBUG
	strcat(m_deviceName, "_d");
#endif

	TestAmp();
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


void dgWorldBase::CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, dgFloat32 timestep)
{
	DG_TRACKTIME_NAMED(GetId());
	dgSolver::CalculateJointForces(cluster, bodyArray, jointArray, timestep);
}


void ScaleElements(index<1> idx, array_view<int, 1> a) restrict(amp)
{
	a[idx] = a[idx] * 10;
}

void dgWorldBase::TestAmp()
{
	std::vector<int> data(5);

	for (int count = 0; count < 5; count++)
	{
		data[count] = count;
	}

	array<int, 1> a(5, data.begin(), data.end());


	parallel_for_each(
		a.extent,
		[=, &a](index<1> idx) restrict(amp)
	{
		ScaleElements(idx, a);
	});


	data = a;
	for (int i = 0; i < 5; i++)
	{
		std::cout << data[i] << "\n";
	}
}
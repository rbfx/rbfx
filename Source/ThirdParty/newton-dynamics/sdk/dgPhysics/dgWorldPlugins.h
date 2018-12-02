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

#ifndef _DG_WORLD_PLUGINS_H_
#define _DG_WORLD_PLUGINS_H_

class dgWorld;
class dgBodyInfo;
class dgJointInfo;
class dgBodyCluster;


class dgWorldPlugin
{
	public:
	dgWorldPlugin(dgWorld* const world, dgMemoryAllocator* const allocator);
	virtual ~dgWorldPlugin();

	virtual const char* GetId() const = 0;
	virtual dgInt32 GetScore() const = 0;
	virtual void CalculateJointForces(const dgBodyCluster& cluster, dgBodyInfo* const bodyArray, dgJointInfo* const jointArray, dgFloat32 timestep) = 0;

	protected:
	dgWorld* m_world;
	dgMemoryAllocator* m_allocator;
	friend class dgWorld;
};

#ifdef __cplusplus 
extern "C"
{
	typedef dgWorldPlugin* (*InitPlugin)(dgWorld* const world, dgMemoryAllocator* const allocator);
}
#endif


class dgWorldPluginModulePair
{
	public:
	dgWorldPluginModulePair (dgWorldPlugin* const plugin, void* module)
		:m_plugin(plugin)
		,m_module(module)
	{
	}
	dgWorldPlugin* m_plugin;
	void* m_module;
};

class dgWorldPluginList: public dgList<dgWorldPluginModulePair>
{
	public:
	dgWorldPluginList(dgMemoryAllocator* const allocator);
	~dgWorldPluginList();

	void LoadPlugins(const char* const path);
	void UnloadPlugins();

	dgListNode* GetFirstPlugin();
	dgListNode* GetCurrentPlugin();
	dgListNode* GetpreferedPlugin();
	dgListNode* GetNextPlugin(dgListNode* const plugin);
	const char* GetPluginId(dgListNode* const plugin);
	void SelectPlugin(dgListNode* const plugin);

	private:
	void LoadVisualStudioPlugins(const char* const path);

	dgListNode* m_currentPlugin;
	dgListNode* m_preferedPlugin;
};


inline dgWorldPlugin::dgWorldPlugin(dgWorld* const world, dgMemoryAllocator* const allocator)
	:m_world(world)
	,m_allocator(allocator)
{
}

inline dgWorldPlugin::~dgWorldPlugin()
{
}


#endif

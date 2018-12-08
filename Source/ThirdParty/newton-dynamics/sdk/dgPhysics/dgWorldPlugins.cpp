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

#include "dgPhysicsStdafx.h"
#include "dgWorld.h"
#include "dgWorldPlugins.h"


	

dgWorldPluginList::dgWorldPluginList(dgMemoryAllocator* const allocator)
	:dgList<dgWorldPluginModulePair>(allocator)
	,m_currentPlugin(NULL)
	,m_preferedPlugin(NULL)
{
}

dgWorldPluginList::~dgWorldPluginList()
{
}


void dgWorldPluginList::LoadVisualStudioPlugins(const char* const plugInPath)
{
#if _MSC_VER > 1700
	char rootPathInPath[2048];
	sprintf(rootPathInPath, "%s/*.dll", plugInPath);

	dgInt32 score = 0;
	dgWorld* const world = (dgWorld*) this;

	// scan for all plugins in this folder
	_finddata_t data;
	intptr_t handle = _findfirst(rootPathInPath, &data);
	if (handle != -1) {
		do {
			sprintf(rootPathInPath, "%s/%s", plugInPath, data.name);
			HMODULE module = LoadLibrary(rootPathInPath);

			if (module) {
				// get the interface function pointer to the Plug in classes
				InitPlugin initModule = (InitPlugin)GetProcAddress(module, "GetPlugin");
				if (initModule) {
					dgWorldPlugin* const plugin = initModule(world, GetAllocator ());
					if (plugin) {
						dgWorldPluginModulePair entry(plugin, module);
						dgListNode* const node = Append(entry);
						dgInt32 pluginValue = plugin->GetScore();
						if (pluginValue > score) {
							score = pluginValue;
							m_preferedPlugin = node; 
						}
					} else {
						FreeLibrary(module);
					}
				} else {
					FreeLibrary(module);
				}
			}

		} while (_findnext(handle, &data) == 0);

		_findclose(handle);
	}
#endif	
}

void dgWorldPluginList::LoadPlugins(const char* const path)
{
#ifndef _NEWTON_USE_DOUBLE
	#ifdef _MSC_VER
		UnloadPlugins();
		LoadVisualStudioPlugins(path);
	#endif
#endif
}

void dgWorldPluginList::UnloadPlugins()
{
#ifdef _MSC_VER
	dgWorldPluginList& pluginsList = *this;
	for (dgWorldPluginList::dgListNode* node = pluginsList.GetFirst(); node; node = node->GetNext()) {
		HMODULE module = (HMODULE)node->GetInfo().m_module;
		FreeLibrary(module);
	}
#endif
	m_currentPlugin = NULL;
	m_preferedPlugin = NULL;
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetCurrentPlugin()
{
	return m_currentPlugin;
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetpreferedPlugin()
{
	return m_preferedPlugin;
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetFirstPlugin()
{
	dgWorldPluginList& list = *this;
	return list.GetFirst();
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetNextPlugin(dgListNode* const plugin)
{
	return plugin->GetNext();
}

const char* dgWorldPluginList::GetPluginId(dgListNode* const pluginNode)
{
	dgWorldPluginModulePair entry(pluginNode->GetInfo());
	dgWorldPlugin* const plugin = entry.m_plugin;
	return plugin->GetId();
}

void dgWorldPluginList::SelectPlugin(dgListNode* const plugin)
{
	m_currentPlugin = plugin;
}

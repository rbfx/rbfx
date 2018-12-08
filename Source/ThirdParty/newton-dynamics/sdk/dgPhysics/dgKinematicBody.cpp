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
#include "dgKinematicBody.h"


dgVector dgKinematicBody::m_dummy (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));


dgKinematicBody::dgKinematicBody()
	:dgBody()
{
	m_collidable = false;
	m_type = m_kinematicBody;
	m_rtti |= m_kinematicBodyRTTI;
}

dgKinematicBody::dgKinematicBody (dgWorld* const world, const dgTree<const dgCollision*, dgInt32>* const collisionNode, dgDeserialize serializeCallback, void* const userData, dgInt32 revisionNumber)
	:dgBody (world, collisionNode, serializeCallback, userData, revisionNumber)
{
	m_collidable = false;
	m_type = m_kinematicBody;
	m_rtti |= m_kinematicBodyRTTI;
}

dgKinematicBody::~dgKinematicBody ()
{
}

void dgKinematicBody::Serialize (const dgTree<dgInt32, const dgCollision*>& collisionRemapId, dgSerialize serializeCallback, void* const userData)
{
	dgBody::Serialize (collisionRemapId, serializeCallback, userData);
}


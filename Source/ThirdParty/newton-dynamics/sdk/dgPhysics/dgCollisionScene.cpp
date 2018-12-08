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
#include "dgBody.h"
#include "dgWorld.h"
#include "dgContact.h"
#include "dgIntersections.h"
#include "dgCollisionScene.h"
#include "dgCollisionInstance.h"


dgCollisionScene::dgCollisionScene (dgWorld* const world)
	:dgCollisionCompound(world)
{
	m_collisionId = m_sceneCollision;
	m_rtti |= dgCollisionScene_RTTI;
}

dgCollisionScene::dgCollisionScene (const dgCollisionScene& source, const dgCollisionInstance* const myInstance)
	:dgCollisionCompound(source, myInstance)
{
	m_rtti |= dgCollisionScene_RTTI;
}

dgCollisionScene::dgCollisionScene (dgWorld* const world, dgDeserialize deserialization, void* const userData, const dgCollisionInstance* const myInstance, dgInt32 revisionNumber)
	:dgCollisionCompound(world, deserialization, userData, myInstance, revisionNumber)
{
	dgAssert (m_rtti | dgCollisionScene_RTTI);
}

dgCollisionScene::~dgCollisionScene()
{
}

dgFloat32 dgCollisionScene::GetBoxMinRadius () const
{
	return dgFloat32 (0.0f);  
}

dgFloat32 dgCollisionScene::GetBoxMaxRadius () const
{
	return dgFloat32 (0.0f);  
}


void dgCollisionScene::MassProperties ()
{
	m_inertia = dgVector (dgFloat32 (0.0f));
	m_centerOfMass = dgVector (dgFloat32 (0.0f));
	m_crossInertia = dgVector (dgFloat32 (0.0f));
}

void dgCollisionScene::CollidePair (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const
{
	const dgNodeBase* stackPool[DG_COMPOUND_STACK_DEPTH];

	dgAssert (proxy.m_contactJoint == pair->m_contact);
	dgContact* const contactJoint = pair->m_contact;
	dgBody* const otherBody = contactJoint->GetBody0();
	dgBody* const sceneBody = contactJoint->GetBody1();
	dgAssert (sceneBody->GetCollision()->GetChildShape() == this);
	dgAssert (sceneBody->GetCollision()->IsType(dgCollision::dgCollisionScene_RTTI));

	dgCollisionInstance* const sceneInstance = sceneBody->m_collision;
	dgCollisionInstance* const otherInstance = otherBody->m_collision;

	dgAssert (sceneInstance->GetChildShape() == this);
	dgAssert (otherInstance->IsType (dgCollision::dgCollisionConvexShape_RTTI));

	const dgContactMaterial* const material = contactJoint->GetMaterial();

	const dgMatrix& myMatrix = sceneInstance->GetGlobalMatrix();
	const dgMatrix& otherMatrix = otherInstance->GetGlobalMatrix();
	dgMatrix matrix (otherMatrix * myMatrix.Inverse());

	const dgVector& hullVeloc = otherBody->m_veloc;
	dgAssert (hullVeloc.m_w == dgFloat32 (0.0f));
	dgFloat32 baseLinearSpeed = dgSqrt (hullVeloc.DotProduct(hullVeloc).GetScalar());

	dgFloat32 timestep = pair->m_timestep;
	dgFloat32 closestDist = dgFloat32 (1.0e10f);
	dgFloat32 separatingDist = dgFloat32 (1.0e10f);
	if (proxy.m_continueCollision && (baseLinearSpeed > dgFloat32 (1.0e-6f))) {
		dgVector p0;
		dgVector p1;
		otherInstance->CalcAABB (matrix, p0, p1);

		const dgVector& hullOmega = otherBody->m_omega;
		dgAssert (hullOmega.m_w == dgFloat32 (0.0f));

		dgFloat32 minRadius = otherInstance->GetBoxMinRadius();
		dgFloat32 maxAngularSpeed = dgSqrt (hullOmega.DotProduct(hullOmega).GetScalar());
		dgFloat32 angularSpeedBound = maxAngularSpeed * (otherInstance->GetBoxMaxRadius() - minRadius);

		dgFloat32 upperBoundSpeed = baseLinearSpeed + dgSqrt (angularSpeedBound);
		dgVector upperBoundVeloc (hullVeloc.Scale (proxy.m_timestep * upperBoundSpeed / baseLinearSpeed));

		dgVector boxDistanceTravelInMeshSpace (myMatrix.UnrotateVector(upperBoundVeloc * otherInstance->m_invScale));

		dgInt32 stack = 1;
		stackPool[0] = m_root;
		dgFastRayTest ray (dgVector (dgFloat32 (0.0f)), boxDistanceTravelInMeshSpace);

		dgFloat32 maxParam = proxy.m_timestep;
		while (stack) {
			stack--;
			const dgNodeBase* const me = stackPool[stack];
			dgAssert (me);

			if (me->BoxIntersect (ray, p0, p1)) {
				if (me->m_type == m_leaf) {
					dgAssert (!me->m_right);
					bool processContacts = true;
					if (material->m_compoundAABBOverlap) {
						//processContacts = material->m_compoundAABBOverlap (*material, sceneBody, me->m_myNode, otherBody, NULL, proxy.m_threadIndex);
						processContacts = material->m_compoundAABBOverlap(*contactJoint, timestep, sceneBody, me->m_myNode, otherBody, NULL, proxy.m_threadIndex);
					}

					if (processContacts) {
						const dgCollisionInstance* const myInstance =  me->GetShape();
						dgCollisionInstance childInstance (*myInstance, myInstance->GetChildShape());
						childInstance.SetGlobalMatrix(childInstance.GetLocalMatrix() * myMatrix);
						proxy.m_instance1 = &childInstance;
						dgInt32 count = pair->m_contactCount;

						proxy.m_timestep = maxParam;
						m_world->SceneChildContacts (pair, proxy);
						// remember to update separating distance
						dgAssert (0);
						//data.m_separatingDistance = dgMin(proxy.m_contactJoint->m_separationDistance, data.m_separatingDistance);
						dgFloat32 param = proxy.m_timestep;
						dgAssert(param >= dgFloat32(0.0f));
						if (param < maxParam) {
							maxParam = param;
						}

						if (pair->m_contactCount > count) {
							dgContactPoint* const buffer = proxy.m_contacts;
							for (dgInt32 i = count; i < pair->m_contactCount; i ++) {
								dgAssert (buffer[i].m_collision0 == proxy.m_instance0);
								if (buffer[i].m_collision1->GetChildShape() == myInstance->GetChildShape()) {
									buffer[i].m_collision1 = myInstance;
								}
							}
						}
						closestDist = dgMin(closestDist, contactJoint->m_closestDistance);
					}

				} else {
					dgAssert (me->m_type == m_node);
					stackPool[stack] = me->m_left;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack] = me->m_right;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));
				}
			}
		}

		proxy.m_timestep = maxParam;
	} else {
		dgVector size;
		dgVector origin;

		otherInstance->CalcObb(origin, size);
		dgOOBBTestData data (matrix, origin, size);
		dgInt32 stack = 1;
		stackPool[0] = m_root;
		while (stack) {

			stack --;
			const dgNodeBase* const me = stackPool[stack];
			dgAssert (me);

			if (me->BoxTest (data)) {
				if (me->m_type == m_leaf) {
					dgAssert (!me->m_right);
					bool processContacts = true;
					if (material->m_compoundAABBOverlap) {
						processContacts = material->m_compoundAABBOverlap (*contactJoint, timestep, sceneBody, me->m_myNode, otherBody, NULL, proxy.m_threadIndex);
					}

					if (processContacts) {
						const dgCollisionInstance* const myInstance =  me->GetShape();
						dgCollisionInstance childInstance (*myInstance, myInstance->GetChildShape());
						childInstance.SetGlobalMatrix(childInstance.GetLocalMatrix() * myMatrix);
						proxy.m_instance1 = &childInstance;
						dgInt32 count = pair->m_contactCount;
						m_world->SceneChildContacts (pair, proxy);
						data.m_separatingDistance = dgMin(proxy.m_contactJoint->m_separationDistance, data.m_separatingDistance);
						if (pair->m_contactCount > count) {
							dgContactPoint* const buffer = proxy.m_contacts;
							for (dgInt32 i = count; i < pair->m_contactCount; i ++) {
								dgAssert (buffer[i].m_collision0 == proxy.m_instance0);
								if (buffer[i].m_collision1->GetChildShape() == myInstance->GetChildShape()) {
									buffer[i].m_collision1 = myInstance;
								}
							}
						} else if (pair->m_contactCount == -1) {
							break;
						}
						closestDist = dgMin(closestDist, contactJoint->m_closestDistance);
					}

				} else {
					dgAssert (me->m_type == m_node);
					stackPool[stack] = me->m_left;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack] = me->m_right;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));
				}
			}
		}
		separatingDist = dgMin (separatingDist, data.m_separatingDistance);
	}
	contactJoint->m_closestDistance = closestDist;
	contactJoint->m_separationDistance = separatingDist;
}


void dgCollisionScene::CollideCompoundPair (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const
{
	const dgNodeBase* stackPool[4 * DG_COMPOUND_STACK_DEPTH][2];

	dgContact* const contactJoint = pair->m_contact;
	dgBody* const myBody = contactJoint->GetBody1();
	dgBody* const otherBody = contactJoint->GetBody0();

	dgAssert (myBody == proxy.m_body1);
	dgAssert (otherBody == proxy.m_body0);

	dgCollisionInstance* const myCompoundInstance = myBody->m_collision;
	dgCollisionInstance* const otherCompoundInstance = otherBody->m_collision;

	dgAssert (myCompoundInstance->GetChildShape() == this);
	dgAssert (otherCompoundInstance->IsType (dgCollision::dgCollisionCompound_RTTI));
	dgCollisionCompound* const otherCompound = (dgCollisionCompound*)otherCompoundInstance->GetChildShape();

	const dgContactMaterial* const material = contactJoint->GetMaterial();

	dgMatrix myMatrix (myCompoundInstance->GetLocalMatrix() * myBody->m_matrix);
	dgMatrix otherMatrix (otherCompoundInstance->GetLocalMatrix() * otherBody->m_matrix);
	dgOOBBTestData data (otherMatrix * myMatrix.Inverse());

	dgInt32 stack = 1;
	stackPool[0][0] = m_root;
	stackPool[0][1] = otherCompound->m_root;

	const dgVector& hullVeloc = otherBody->m_veloc;
	dgAssert (hullVeloc.m_w == dgFloat32 (0.0f));
	dgFloat32 baseLinearSpeed = dgSqrt (hullVeloc.DotProduct(hullVeloc).GetScalar());

	dgFloat32 timestep = pair->m_timestep;
	dgFloat32 closestDist = dgFloat32 (1.0e10f);
	dgFloat32 separatingDist = dgFloat32 (1.0e10f);
	if (proxy.m_continueCollision && (baseLinearSpeed > dgFloat32 (1.0e-6f))) {
		dgAssert (0);
	} else {
		while (stack) {
			stack --;
			const dgNodeBase* const me = stackPool[stack][0];
			const dgNodeBase* const other = stackPool[stack][1];

			dgAssert (me && other);

			if (me->BoxTest (data, other)) {

				if ((me->m_type == m_leaf) && (other->m_type == m_leaf)) {
					dgAssert (!me->m_right);

					bool processContacts = true;
					if (material->m_compoundAABBOverlap) {
						processContacts = material->m_compoundAABBOverlap (*contactJoint, timestep, myBody, me->m_myNode, otherBody, other->m_myNode, proxy.m_threadIndex);
					}

					if (processContacts) {
						const dgCollisionInstance* const mySrcInstance =  me->GetShape();
						const dgCollisionInstance* const otherSrcInstance =  other->GetShape();
						dgCollisionInstance childInstance (*me->GetShape(), me->GetShape()->GetChildShape());
						dgCollisionInstance otherInstance (*other->GetShape(), other->GetShape()->GetChildShape());

						childInstance.SetGlobalMatrix(childInstance.GetLocalMatrix() * myMatrix);
						otherInstance.SetGlobalMatrix(otherInstance.GetLocalMatrix() * otherMatrix);
						proxy.m_instance1 = &childInstance;
						proxy.m_instance0 = &otherInstance;

						dgInt32 count = pair->m_contactCount;
						m_world->SceneChildContacts (pair, proxy);
						if (pair->m_contactCount > count) {
							dgContactPoint* const buffer = proxy.m_contacts;
							for (dgInt32 i = count; i < pair->m_contactCount; i ++) {
								if (buffer[i].m_collision1->GetChildShape() == otherSrcInstance->GetChildShape()) {
									dgAssert(buffer[i].m_collision0->GetChildShape() == mySrcInstance->GetChildShape());
									buffer[i].m_collision0 = mySrcInstance;
									buffer[i].m_collision1 = otherSrcInstance;
								} else {
									dgAssert(buffer[i].m_collision1->GetChildShape() == mySrcInstance->GetChildShape());
									buffer[i].m_collision1 = mySrcInstance;
									buffer[i].m_collision0 = otherSrcInstance;
								}
							}
						}

						closestDist = dgMin(closestDist, contactJoint->m_closestDistance);
						data.m_separatingDistance = dgMin(proxy.m_contactJoint->m_separationDistance, data.m_separatingDistance);
					}

				} else if (me->m_type == m_leaf) {
					dgAssert (other->m_type == m_node);

					stackPool[stack][0] = me;
					stackPool[stack][1] = other->m_left;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack][0] = me;
					stackPool[stack][1] = other->m_right;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));


				} else if (other->m_type == m_leaf) {
					dgAssert (me->m_type == m_node);

					stackPool[stack][0] = me->m_left;
					stackPool[stack][1] = other;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack][0] = me->m_right;
					stackPool[stack][1] = other;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));
				} else {
					dgAssert (me->m_type == m_node);
					dgAssert (other->m_type == m_node);

					stackPool[stack][0] = me->m_left;
					stackPool[stack][1] = other->m_left;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack][0] = me->m_left;
					stackPool[stack][1] = other->m_right;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack][0] = me->m_right;
					stackPool[stack][1] = other->m_left;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));

					stackPool[stack][0] = me->m_right;
					stackPool[stack][1] = other->m_right;
					stack++;
					dgAssert (stack < dgInt32 (sizeof (stackPool) / sizeof (dgNodeBase*)));
				}
			}
		}
		separatingDist = dgMin (separatingDist, data.m_separatingDistance);
	}
	contactJoint->m_closestDistance = closestDist;
	contactJoint->m_separationDistance = separatingDist;
}


void dgCollisionScene::Serialize(dgSerialize callback, void* const userData) const
{
	dgCollisionCompound::Serialize(callback, userData);
}
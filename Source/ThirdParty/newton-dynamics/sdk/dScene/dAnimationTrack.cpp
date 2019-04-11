/////////////////////////////////////////////////////////////////////////////
// Name:        dAnimationTrack.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 08:02:08
// RCS-ID:      
// Copyright:   Copyright (c) <2010> <Newton Game Dynamics>
// License:     
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely
/////////////////////////////////////////////////////////////////////////////


#include "dSceneStdafx.h"
#include "dScene.h"
#include "dAnimationTrack.h"
#include <tinyxml.h>


D_IMPLEMENT_CLASS_NODE(dAnimationTrack);

dAnimationTrack::dAnimationTrack(dScene* const world) 
	:dNodeInfo () 
{
	SetName ("animationLayers");
}

dAnimationTrack::dAnimationTrack()
	:dNodeInfo () 
{
	SetName ("animationLayers");
}

dAnimationTrack::~dAnimationTrack(void)
{
}

void dAnimationTrack::AddScale(dFloat time, dFloat x, dFloat y, dFloat z)
{
	dCurveValue& value = m_scale.Append()->GetInfo();
	value.m_x = x;
	value.m_y = y;
	value.m_z = z;
	value.m_time = time;
}

void dAnimationTrack::AddPosition(dFloat time, dFloat x, dFloat y, dFloat z)
{
	dCurveValue& value = m_position.Append()->GetInfo();
	value.m_x = x;
	value.m_y = y;
	value.m_z = z;
	value.m_time = time;
}

void dAnimationTrack::AddRotation(dFloat time, dFloat x, dFloat y, dFloat z)
{
	dCurveValue& value = m_rotation.Append()->GetInfo();
	value.m_x = x;
	value.m_y = y;
	value.m_z = z;
	value.m_time = time;
}

void dAnimationTrack::AddKeyframe(dFloat time, const dMatrix& matrix)
{
	dVector scale;
	dVector euler0;
	dVector euler1;
	dMatrix transform;
	dMatrix eigenScaleAxis;
 	matrix.PolarDecomposition(transform, scale, eigenScaleAxis);
	transform.GetEulerAngles(euler0, euler1);

	AddScale(time, scale.m_x, scale.m_y, scale.m_z);
	AddPosition(time, matrix.m_posit.m_x, matrix.m_posit.m_y, matrix.m_posit.m_z);
	AddRotation(time, euler0.m_x, euler0.m_y, euler0.m_z);
//dTrace(("%d %f %f %f\n", m_rotation.GetCount(), euler0.m_x * dRadToDegree, euler0.m_y * dRadToDegree, euler0.m_z * dRadToDegree));
}

void dAnimationTrack::ResampleAnimation()
{
	dFloat period = m_scale.GetLast()->GetInfo().m_time;
	dFloat t1 = m_position.GetLast()->GetInfo().m_time;
	period = dMax(period, t1);
	dFloat t2 = m_rotation.GetLast()->GetInfo().m_time;
	period = dMax(period, t2);

	dCurve scaleCurve;
	dCurve positionCurve;
	dCurve rotationCurve;
	dFloat acc = 0.0f;
	do {
		dCurveValue scale(m_scale.Evaluate(acc));
		dCurveValue position(m_position.Evaluate(acc));
		dCurveValue rotation(m_rotation.Evaluate(acc));

		scaleCurve.Append(scale);
		positionCurve.Append(position);
		rotationCurve.Append(rotation);

		acc += 1.0f / 60.0f;
	} while (acc < period);

	dCurveValue scale(m_scale.Evaluate(period));
	dCurveValue position(m_position.Evaluate(period));
	dCurveValue rotation(m_rotation.Evaluate(period));
	scaleCurve.Append(scale);
	positionCurve.Append(position);
	rotationCurve.Append(rotation);

	m_scale.RemoveAll();
	m_position.RemoveAll();
	m_rotation.RemoveAll();
	for (dCurve::dListNode* ptr = scaleCurve.GetFirst(); ptr; ptr = ptr->GetNext()) {
		m_scale.Append(ptr->GetInfo());
	}

	for (dCurve::dListNode* ptr = positionCurve.GetFirst(); ptr; ptr = ptr->GetNext()) {
		m_position.Append(ptr->GetInfo());
	}

	for (dCurve::dListNode* ptr = rotationCurve.GetFirst(); ptr; ptr = ptr->GetNext()) {
		m_rotation.Append(ptr->GetInfo());
	}
}

void dAnimationTrack::FreezeScale(const dMatrix& parentMatrix)
{
	ResampleAnimation();

	dCurve::dListNode* scaleNode = m_scale.GetFirst();
	dCurve::dListNode* positNode = m_position.GetFirst();
	for (dCurve::dListNode* rotationNode = m_rotation.GetFirst(); rotationNode; rotationNode = rotationNode->GetNext()) {
		if (m_position.GetCount() && m_rotation.GetCount()) {
			dVector euler0;
			dVector euler1;
			dCurveValue& scaleValue = scaleNode->GetInfo();
			dCurveValue& positValue = positNode->GetInfo();
			dCurveValue& rotationValue = rotationNode->GetInfo();

			dMatrix scaleMatrix(dGetIdentityMatrix());
			scaleMatrix[0][0] = scaleValue.m_x;
			scaleMatrix[1][1] = scaleValue.m_y;
			scaleMatrix[2][2] = scaleValue.m_z;
			dMatrix matrix(scaleMatrix * dPitchMatrix(rotationValue.m_x) * dYawMatrix(rotationValue.m_y) * dRollMatrix(rotationValue.m_z));
			matrix.m_posit = dVector(positValue.m_x, positValue.m_y, positValue.m_z, 1.0f);
			dMatrix transform(matrix * parentMatrix);

			dMatrix stretchAxis;
			dVector scale(0.0f);
			transform.PolarDecomposition(matrix, scale, stretchAxis);
			matrix.GetEulerAngles(euler0, euler1);

			scaleValue.m_x = dFloat(1.0f);
			scaleValue.m_y = dFloat(1.0f);
			scaleValue.m_z = dFloat(1.0f);

			rotationValue.m_x = euler0.m_x;
			rotationValue.m_y = euler0.m_y;
			rotationValue.m_z = euler0.m_z;

			positValue.m_x = matrix.m_posit.m_x;
			positValue.m_y = matrix.m_posit.m_y;
			positValue.m_z = matrix.m_posit.m_z;

			positNode = positNode->GetNext();
			scaleNode = scaleNode->GetNext();
		}
	}

	OptimizeCurves();
}

void dAnimationTrack::BakeTransform(const dMatrix& transform)
{
	ResampleAnimation();

	dMatrix invert(transform.Inverse4x4());
	dCurve::dListNode* scaleNode = m_scale.GetFirst();
	dCurve::dListNode* positNode = m_position.GetFirst();
	for (dCurve::dListNode* rotationNode = m_rotation.GetFirst(); rotationNode; rotationNode = rotationNode->GetNext()) {
		dVector euler0;
		dVector euler1;
			
		dCurveValue& scaleValue = scaleNode->GetInfo();
		dCurveValue& positValue = positNode->GetInfo();
		dCurveValue& rotationValue = rotationNode->GetInfo();

		dMatrix scaleMatrix(dGetIdentityMatrix());
		scaleMatrix[0][0] = scaleValue.m_x;
		scaleMatrix[1][1] = scaleValue.m_y;
		scaleMatrix[2][2] = scaleValue.m_z;
		dMatrix m(scaleMatrix * dPitchMatrix(rotationValue.m_x) * dYawMatrix(rotationValue.m_y) * dRollMatrix(rotationValue.m_z));
		m.m_posit = dVector(positValue.m_x, positValue.m_y, positValue.m_z, 1.0f);
		dMatrix matrix(invert * m * transform);

		dVector scale;
		dMatrix output;
		dMatrix eigenScaleAxis;
		matrix.PolarDecomposition(output, scale, eigenScaleAxis);
		output.GetEulerAngles(euler0, euler1);
//dTrace(("%d %f %f %f\n", m_rotation.GetCount(), euler0.m_x * dRadToDegree, euler0.m_y * dRadToDegree, euler0.m_z * dRadToDegree));

		scaleValue.m_x = scale.m_x;
		scaleValue.m_y = scale.m_y;
		scaleValue.m_z = scale.m_z;

		rotationValue.m_x = euler0.m_x;
		rotationValue.m_y = euler0.m_y;
		rotationValue.m_z = euler0.m_z;

		positValue.m_x = output.m_posit.m_x;
		positValue.m_y = output.m_posit.m_y;
		positValue.m_z = output.m_posit.m_z;

		positNode = positNode->GetNext();
		scaleNode = scaleNode->GetNext();
	}

	OptimizeCurves();
}

dFloat dAnimationTrack::Interpolate(dFloat x0, dFloat t0, dFloat x1, dFloat t1, dFloat t) const
{
	return x0 + (x1 - x0) * (t - t0) / (t1 - t0);
}

void dAnimationTrack::OptimizeCurve(dList<dCurveValue>& curve)
{
	const dFloat tol = 5.0e-5f;
	const dFloat tol2 = tol * tol;
	for (dCurve::dListNode* node0 = curve.GetFirst(); node0->GetNext(); node0 = node0->GetNext()) {
		const dCurveValue& value0 = node0->GetInfo();
		for (dCurve::dListNode* node1 = node0->GetNext()->GetNext(); node1; node1 = node1->GetNext()) {
			const dCurveValue& value1 = node1->GetPrev()->GetInfo();
			const dCurveValue& value2 = node1->GetInfo();
			dVector p1(value1.m_x, value1.m_y, value1.m_z, dFloat(0.0f));
			dVector p2(value2.m_x, value2.m_y, value2.m_z, dFloat(0.0f));

			dFloat dist_x = value1.m_x - Interpolate(value0.m_x, value0.m_time, value2.m_x, value2.m_time, value1.m_time);
			dFloat dist_y = value1.m_y - Interpolate(value0.m_y, value0.m_time, value2.m_y, value2.m_time, value1.m_time);
			dFloat dist_z = value1.m_z - Interpolate(value0.m_z, value0.m_time, value2.m_z, value2.m_time, value1.m_time);

			dVector err(dist_x, dist_y, dist_z, 0.0f);
			dFloat mag2 = err.DotProduct3(err);
			if (mag2 > tol2) {
				break;
			}
			curve.Remove(node1->GetPrev());
		}
	}
}

dFloat dAnimationTrack::FixAngleAlias(dFloat angleA, dFloat angleB) const
{
	dFloat sinA = dSin(angleA);
	dFloat cosA = dCos(angleA);
	dFloat sinB = dSin(angleB);
	dFloat cosB = dCos(angleB);

	dFloat num = sinB * cosA - cosB * sinA;
	dFloat den = cosA * cosB + sinA * sinB;
	angleB = angleA + dAtan2(num, den);
	return angleB;
}

void dAnimationTrack::OptimizeCurves()
{
	if (m_scale.GetCount()) {
		OptimizeCurve(m_scale);
	}
	if (m_position.GetCount()) {
		OptimizeCurve(m_position);
	}
	if (m_rotation.GetCount()) {
		for (dCurve::dListNode* node = m_rotation.GetFirst(); node->GetNext(); node = node->GetNext()) {
			const dCurveValue& value0 = node->GetInfo();
			dCurveValue& value1 = node->GetNext()->GetInfo();
			value1.m_x = FixAngleAlias(value0.m_x, value1.m_x);
			value1.m_y = FixAngleAlias(value0.m_y, value1.m_y);
			value1.m_z = FixAngleAlias(value0.m_z, value1.m_z);
//dTrace(("%d %f %f %f\n", m_rotation.GetCount(), value0.m_x * dRadToDegree, value0.m_y * dRadToDegree, value0.m_z * dRadToDegree));
		}

		OptimizeCurve(m_rotation);
	}
}

void dAnimationTrack::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);
	dAssert(m_scale.GetCount() >= 2);
	dAssert(m_rotation.GetCount() >= 2);
	dAssert(m_position.GetCount() >= 2);

	if (m_scale.GetCount()) {
		TiXmlElement* const scaleKeys = new TiXmlElement("scaleKeyframes");
		rootNode->LinkEndChild(scaleKeys);

		int bufferSizeInBytes = 3 * 12 * m_scale.GetCount() * sizeof(dFloat);
		char* const buffer = dAlloca(char, bufferSizeInBytes);
		dFloat* const time = dAlloca(dFloat, m_scale.GetCount());
		dFloat* const points = dAlloca(dFloat, 3 * m_scale.GetCount());

		int count = 0;
		for (dCurve::dListNode* node = m_scale.GetFirst(); node; node = node->GetNext()) {
			const dCurveValue& value = node->GetInfo();
			time[count] = value.m_time;
			points[count * 3 + 0] = value.m_x;
			points[count * 3 + 1] = value.m_y;
			points[count * 3 + 2] = value.m_z;
			count++;
		}

		TiXmlElement* const timeLine = new TiXmlElement("timeLine");
		scaleKeys->LinkEndChild(timeLine);
		dFloatArrayToString(time, count, buffer, bufferSizeInBytes);
		timeLine->SetAttribute("float", count);
		timeLine->SetAttribute("floats", buffer);

		TiXmlElement* const positions = new TiXmlElement("scale");
		scaleKeys->LinkEndChild(positions);
		dFloatArrayToString(points, 3 * count, buffer, bufferSizeInBytes);
		positions->SetAttribute("float3", count);
		positions->SetAttribute("floats", buffer);
	}

	if (m_position.GetCount()) {

		TiXmlElement* const positionKeys = new TiXmlElement("positionKeyframes");
		rootNode->LinkEndChild(positionKeys);

		int bufferSizeInBytes = 3 * 12 * m_position.GetCount() * sizeof(dFloat);
		char* const buffer = dAlloca(char, bufferSizeInBytes);
		dFloat* const time = dAlloca(dFloat, m_position.GetCount());
		dFloat* const points = dAlloca(dFloat, 3 * m_position.GetCount());
		
		int count = 0; 
		for (dCurve::dListNode* node = m_position.GetFirst(); node; node = node->GetNext()) {
			const dCurveValue& value = node->GetInfo();
			time[count] = value.m_time;
			points[count * 3 + 0] = value.m_x;
			points[count * 3 + 1] = value.m_y;
			points[count * 3 + 2] = value.m_z;
			count++;
		}

		TiXmlElement* const timeLine = new TiXmlElement("timeLine");
		positionKeys->LinkEndChild(timeLine);
		dFloatArrayToString(time, count, buffer, bufferSizeInBytes);
		timeLine->SetAttribute("float", count);
		timeLine->SetAttribute("floats", buffer);

		TiXmlElement* const positions = new TiXmlElement("position");
		positionKeys->LinkEndChild(positions);
		dFloatArrayToString(points, 3 * count, buffer, bufferSizeInBytes);
		positions->SetAttribute("float3", count);
		positions->SetAttribute("floats", buffer);
	}

	if (m_rotation.GetCount()) {
		TiXmlElement* const rotationKeys = new TiXmlElement("rotationKeyframes");
		rootNode->LinkEndChild(rotationKeys);

		int bufferSizeInBytes = 3 * 12 * m_rotation.GetCount() * sizeof(dFloat);
		char* const buffer = dAlloca(char, bufferSizeInBytes);
		dFloat* const time = dAlloca(dFloat, m_rotation.GetCount());
		dFloat* const points = dAlloca(dFloat, 3 * m_rotation.GetCount());

		int count = 0;
		for (dCurve::dListNode* node = m_rotation.GetFirst(); node; node = node->GetNext()) {
			const dCurveValue& value = node->GetInfo();
			time[count] = value.m_time;
			points[count * 3 + 0] = value.m_x;
			points[count * 3 + 1] = value.m_y;
			points[count * 3 + 2] = value.m_z;
			count++;
		}

		TiXmlElement* const timeLine = new TiXmlElement("timeLine");
		rotationKeys->LinkEndChild(timeLine);
		dFloatArrayToString(time, count, buffer, bufferSizeInBytes);
		timeLine->SetAttribute("float", count);
		timeLine->SetAttribute("floats", buffer);

		TiXmlElement* const positions = new TiXmlElement("angles");
		rotationKeys->LinkEndChild(positions);
		dFloatArrayToString(points, 3 * count, buffer, bufferSizeInBytes);
		positions->SetAttribute("float3", count);
		positions->SetAttribute("floats", buffer);
	}
}

bool dAnimationTrack::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	TiXmlElement* const scaleKeyframes = (TiXmlElement*)rootNode->FirstChild("scaleKeyframes");
	if (scaleKeyframes) {

		TiXmlElement* const timeLineElement = (TiXmlElement*)scaleKeyframes->FirstChild("timeLine");
		TiXmlElement* const positionElement = (TiXmlElement*)scaleKeyframes->FirstChild("scale");

		int keyFramesCount;
		timeLineElement->Attribute("float", &keyFramesCount);

		dFloat* const timeline = dAlloca(dFloat, keyFramesCount);
		dFloat* const points = dAlloca(dFloat, 3 * keyFramesCount);

		dStringToFloatArray(timeLineElement->Attribute("floats"), timeline, keyFramesCount);
		dStringToFloatArray(positionElement->Attribute("floats"), points, 3 * keyFramesCount);

		for (int i = 0; i < keyFramesCount; i++) {
			AddScale(timeline[i], points[i * 3 + 0], points[i * 3 + 1], points[i * 3 + 2]);
		}
	}


	TiXmlElement* const positionKeyframes = (TiXmlElement*)rootNode->FirstChild("positionKeyframes");
	if (positionKeyframes) {

		TiXmlElement* const timeLineElement = (TiXmlElement*)positionKeyframes->FirstChild("timeLine");
		TiXmlElement* const positionElement = (TiXmlElement*)positionKeyframes->FirstChild("position");

		int keyFramesCount;
		timeLineElement->Attribute("float", &keyFramesCount);

		dFloat* const timeline = dAlloca(dFloat, keyFramesCount);
		dFloat* const points = dAlloca(dFloat, 3 * keyFramesCount);

		dStringToFloatArray(timeLineElement->Attribute("floats"), timeline, keyFramesCount);
		dStringToFloatArray(positionElement->Attribute("floats"), points, 3 * keyFramesCount);

		for (int i = 0; i < keyFramesCount; i ++) {
			AddPosition(timeline[i], points[i * 3 + 0], points[i * 3 + 1], points[i * 3 + 2]);
		}
	}

	TiXmlElement* const rotationKeyframes = (TiXmlElement*)rootNode->FirstChild("rotationKeyframes");
	if (rotationKeyframes) {

		TiXmlElement* const timeLineElement = (TiXmlElement*)rotationKeyframes->FirstChild("timeLine");
		TiXmlElement* const positionElement = (TiXmlElement*)rotationKeyframes->FirstChild("angles");

		int keyFramesCount;
		timeLineElement->Attribute("float", &keyFramesCount);

		dFloat* const timeline = dAlloca(dFloat, keyFramesCount);
		dFloat* const points = dAlloca(dFloat, 3 * keyFramesCount);

		dStringToFloatArray(timeLineElement->Attribute("floats"), timeline, keyFramesCount);
		dStringToFloatArray(positionElement->Attribute("floats"), points, 3 * keyFramesCount);

		for (int i = 0; i < keyFramesCount; i++) {
			AddRotation(timeline[i], points[i * 3 + 0], points[i * 3 + 1], points[i * 3 + 2]);
		}
	}

	return true;
}

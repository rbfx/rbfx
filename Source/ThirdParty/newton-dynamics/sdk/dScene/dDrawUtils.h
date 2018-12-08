/////////////////////////////////////////////////////////////////////////////
// Name:        dDrawUtils.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 07:45:05
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


#ifndef __D_DRAW_UTILS_H__
#define __D_DRAW_UTILS_H__

#include <dStdAfxMath.h>
#include <dMathDefines.h>
#include <dVector.h>
#include <dMatrix.h>
#include <dQuaternion.h>

int CreateCone (dVector* const points, dVector* const normals, int segments, dFloat radius, dFloat height, int maxPoints);
int CreateCylinder (dVector* const points, dVector* const normals, int segments, dFloat radius, dFloat height, int maxPoints);


void Draw3DArrow (const dMatrix& matrix, int segments, dFloat radius, dFloat height, const dVector& color);
void Draw3DCylinder (const dMatrix& matrix, int segments, dFloat radius, dFloat height, const dVector& color);

#endif
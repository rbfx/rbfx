/* Copyright (c) <2003-2016> <Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


#ifndef __dNewtonLuaCompiler_h__
#define __dNewtonLuaCompiler_h__


#include <dCRC.h>
#include <dTree.h>
#include <dList.h>
#include <dRtti.h>
#include <dRefCounter.h>
#include <dContainersStdAfx.h>
#include "dNewtonLuaParcer.h"


class dNewtonLuaLex;

class dNewtonLuaCompiler: public dNewtonLuaParcer
{
	public:
	dNewtonLuaCompiler();
	virtual ~dNewtonLuaCompiler();

	int CompileSource (const char* const sourceFileName);

	protected:
	class dLuaClosure: public dCIL
	{
		public:
		dLuaClosure();
		~dLuaClosure();

		void RemoveAll();
		dLuaClosure* AddClosure(dLuaClosure* const parent);

		private:
		dLuaClosure* m_parent;
		dList<dLuaClosure> m_children;
		dString m_returnLabel;
		dString m_returnVariable;
		dList<dCILInstrMove*> m_argumnets;
		dList<dCILInstrLocal*> m_localVariables;
		friend class dNewtonLuaCompiler;
	};

	void CloseFunctionDeclaration();
	dUserVariable EmitBlockBeginning();

	dUserVariable EmitLabel();
	dUserVariable EmitReturn(const dUserVariable& expression);
	dUserVariable EmitLoadString(const dUserVariable& constName);
	dUserVariable EmitLoadVariable(const dUserVariable& varName);
	dUserVariable EmitLoadConstant(const dUserVariable& constName);
	
	dUserVariable EmitLocalVariableDeclaration(const dUserVariable& nameList);
	dUserVariable EmitParametersToLocalVariables(const dUserVariable& parameterList);
	dUserVariable EmitFunctionCall(const dUserVariable& functionName, const dUserVariable& argumentsList);
	dUserVariable EmitFunctionDeclaration(const dUserVariable& name, const dUserVariable& parameterList);
	dUserVariable EmitAssigmentStatement(const dUserVariable& nameList, const dUserVariable& expressionList);
	dUserVariable EmitIf(const dUserVariable& expression, const dUserVariable& thenBlock, const dUserVariable& elseBlock);
	dUserVariable EmitBinaryExpression(const dUserVariable& arg0, const dUserVariable& binaryOperator, const dUserVariable& arg1);
	dUserVariable EmitFor(const dUserVariable& iterName, const dUserVariable& startLoopLabel, const dUserVariable& testExpression, const dUserVariable& stepExpression, const dUserVariable& block);

	void FixUnitializedReturnVariable();

	dLuaClosure m_closures;
	dLuaClosure* m_currentClosure;

	friend class dNewtonLuaParcer;
};

#endif

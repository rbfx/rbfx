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

#ifndef _dRegisterInterferenceGraph_H_
#define _dRegisterInterferenceGraph_H_


#include "dCILstdafx.h"
#include "dCILInstr.h"


class dRegisterInterferenceNode;

class dRegisterInterferenceNodeEdge
{
	public:
	dRegisterInterferenceNodeEdge (dTree<dRegisterInterferenceNode, dString>::dTreeNode* const m_incidentNode)
		//:m_twin(NULL)
		:m_incidentNode(m_incidentNode)
		,m_mark (false)
	{
	}
	
	dTree<dRegisterInterferenceNode, dString>::dTreeNode* m_incidentNode;
	bool m_mark;
};

class dRegisterInterferenceNode
{
	public: 
	dRegisterInterferenceNode()
		:m_name()
		,m_interferanceEdge()
		,m_registerIndex (-1)
		,m_registerIndexBase(0)
	{
	}

	dRegisterInterferenceNodeEdge* FindEdge(const dString& var);

	dString m_name;
	dList<dRegisterInterferenceNodeEdge> m_interferanceEdge;
	int m_registerIndex;
	int m_registerIndexBase;
};


class dRegisterInterferenceGraph: public dTree<dRegisterInterferenceNode, dString>
{
	public: 
	class dCoalescedNodePair
	{
		public:	
		dCoalescedNodePair (dTreeNode* const nodeA, dTreeNode* const nodeB)
			:m_nodeA (nodeA)
			,m_nodeB (nodeB)
		{
		}
		dTreeNode* m_nodeA;
		dTreeNode* m_nodeB;
	};


	class dVariableSpillPriority
	{
		public:
		dVariableSpillPriority()
			:m_useCount(0)
			,m_loopUseCount(0)
		{
		}

		int m_useCount;
		int m_loopUseCount;
	};

	dRegisterInterferenceGraph (dBasicBlocksGraph* const graph, int registerCount);
	dString GetRegisterName(const dString& varName) const;

	private:
	void Build();
	int ColorGraph();
	bool RemoveNop();
	void AllocateRegisters();
	bool RemoveRedundantJumps ();
	void ApplyDeadCodeElimination();
//	dTreeNode* GetBestNode(int& edgeCount);
	int GetRegisterIndex(const dString& varName) const;
	bool IsSpilledVariable(const dString& name) const;

	dBasicBlocksGraph* m_graph;
};

#endif
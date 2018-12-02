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


#include "dCILstdafx.h"
#include "dCILInstrBranch.h"
#include "dBasicBlocksGraph.h"
#include "dCILInstrLoadStore.h"
#include "dConditionalConstantPropagationSolver.h"

dConditionalConstantPropagationSolver::dInstructionMap::dInstructionMap():dTree<int, dCILInstr*>()
{
}

dConditionalConstantPropagationSolver::dVariable::dVariable(dCILInstr* const instruction, dCILInstr::dArg* const variable)
	:m_type(m_undefined)
	,m_constValue("")
	,m_instruction(instruction)
	,m_variable(variable)
{
}

dConditionalConstantPropagationSolver::dVariable& dConditionalConstantPropagationSolver::dVariable::operator=(const dVariable& copy)
{
	dAssert(0);
	return *this;
}

dConditionalConstantPropagationSolver::dBlockEdgeKey::dBlockEdgeKey(const dBasicBlock* const blockHigh, const dBasicBlock* const blockLow)
	:m_blockHigh(blockHigh)
	,m_blockLow(blockLow)
{
}

bool dConditionalConstantPropagationSolver::dBlockEdgeKey::operator<(const dBlockEdgeKey& src) const
{
	if (m_blockHigh < src.m_blockHigh) {
		return true;
	}
	return m_blockLow < src.m_blockLow;
}

bool dConditionalConstantPropagationSolver::dBlockEdgeKey::operator>(const dBlockEdgeKey& src) const
{
	if (m_blockHigh > src.m_blockHigh) {
		return true;
	}
	return m_blockLow > src.m_blockLow;
}

dConditionalConstantPropagationSolver::dConditionalConstantPropagationSolver (dBasicBlocksGraph* const graph)
	:m_graph(graph)
{
	for (dCIL::dListNode* node = m_graph->m_begin; node != m_graph->m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		dCILInstr::dArg* const variable = instruction->GetGeneratedVariable();
		if (variable) {
			m_variablesList.Insert(dVariable(instruction, variable), variable->m_label);
		}

		dList<dCILInstr::dArg*> variablesList;
		instruction->GetUsedVariables(variablesList);
		for (dList<dCILInstr::dArg*>::dListNode* varNode = variablesList.GetFirst(); varNode; varNode = varNode->GetNext()) {
			dCILInstr::dArg* const arg = varNode->GetInfo();
			dTree<dInstructionMap, dString>::dTreeNode* bucketNode = m_uses.Find(arg->m_label);
			if (!bucketNode) {
				bucketNode =  m_uses.Insert(arg->m_label);
			}
			bucketNode->GetInfo().Insert (instruction);
		}
	}
/*
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		if (instruction->GetAsArgument()) {
			dAssert (0);
	//instruction->Trace();
			dList<dCILInstr::dArg*> variables;
			instruction->GetUsedVariables(variables);
			for (dList<dCILInstr::dArg*>::dListNode* varNode = variables.GetFirst(); varNode; varNode = varNode->GetNext()) {
				dCILInstr::dArg* const variable = varNode->GetInfo();
				dTree<dConstantPropagationVariable, dString>::dTreeNode* const varNodeEntry = variablesList.Insert(dConstantPropagationVariable(instruction, variable), variable->m_label);
				dAssert (varNodeEntry);
				varNodeEntry->GetInfo().m_value = dConstantPropagationVariable::m_varyngValue;
			}
		}
	}

	dTree<int, dBlockEdgeKey> blockEdges;
	for (dBasicBlocksGraph::dListNode* node = GetFirst(); node; node = node->GetNext()) {
		const dBasicBlock* const block = &node->GetInfo();
//block->Trace();
		for (dList<const dBasicBlock*>::dListNode* succNode = block->m_successors.GetFirst(); succNode; succNode = succNode->GetNext()) {
			const dBasicBlock* const succ = succNode->GetInfo();
//succ->Trace();
			blockEdges.Insert(0, dBlockEdgeKey(block, succ));
		}
	}
*/
}

void dConditionalConstantPropagationSolver::Trace()
{
	for (dCIL::dListNode* node = m_graph->m_begin; node != m_graph->m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		dList<dCILInstr::dArg*> variablesList;
		instruction->GetUsedVariables(variablesList);
		if (variablesList.GetCount()) {
			char text[4096];
			instruction->Serialize(text);
			dString str (text);
			for (dList<dCILInstr::dArg*>::dListNode* varNode = variablesList.GetFirst(); varNode; varNode = varNode->GetNext()) {
				dCILInstr::dArg* const arg = varNode->GetInfo();
				dAssert (m_variablesList.Find(arg->m_label));
				dVariable& variable = m_variablesList.Find(arg->m_label)->GetInfo();
				if (variable.m_type == dVariable::m_constant) {
					int index = str.Find (arg->m_label);
					if (index > 0) {
						str.Replace(index, arg->m_label.Size(), variable.m_constValue);
					}
				}
			}

			dTrace ((str.GetStr()));
		} else {
			instruction->Trace();
		}
	}

	dTrace (("\n"));
}

void dConditionalConstantPropagationSolver::UpdateLatice (const dCILInstr::dArg& arg, const dString& newValue, dVariable::dValueTypes type)
{
	dAssert (m_variablesList.Find(arg.m_label));
	dVariable& variable = m_variablesList.Find(arg.m_label)->GetInfo();
	if (type != variable.m_type) {
		variable.m_type = type;
		variable.m_constValue = newValue;
		
		dTree<dInstructionMap, dString>::dTreeNode* const usesNode = m_uses.Find(arg.m_label);
		if (usesNode) {
			dInstructionMap::Iterator iter(usesNode->GetInfo());
			for (iter.Begin(); iter; iter++) {
				dCILInstr* const instruction = iter.GetKey();
				m_instructionsWorklist.Append (instruction);
			}
		}
	}
}


bool dConditionalConstantPropagationSolver::Solve ()
{
	m_blockWorklist.Append(&m_graph->GetFirst()->GetInfo());

	while (m_blockWorklist.GetCount() || m_instructionsWorklist.GetCount()) {
		while (m_instructionsWorklist.GetCount()) {
			dList<dCILInstr*>::dListNode* const nodeOuter = m_instructionsWorklist.GetFirst();
			dCILInstr* const instruction = nodeOuter->GetInfo();
			m_instructionsWorklist.Remove(nodeOuter);
			instruction->ApplyConditionalConstantPropagationSSA(*this);
		}

		while (m_blockWorklist.GetCount()) {
			dList<dBasicBlock*>::dListNode* const nodeOuter = m_blockWorklist.GetFirst();
			dBasicBlock* const block = nodeOuter->GetInfo();
			m_blockWorklist.Remove(nodeOuter);
			bool doneOuter = false;
			for (dCIL::dListNode* node = block->m_begin; !doneOuter; node = node->GetNext()) {
				doneOuter = (node == block->m_end);
				dCILInstrPhy* const phy = node->GetInfo()->GetAsPhi();
				if (phy) {
					phy->ApplyConditionalConstantPropagationSSA (*this);
				}
			}

			if (!m_visited.Find(block)) {
				m_visited.Insert(block);
				bool done = false;
				for (dCIL::dListNode* node = block->m_begin; !done; node = node->GetNext()) {
					done = (node == block->m_end);
					dCILInstr* const instruction = node->GetInfo();
					instruction->ApplyConditionalConstantPropagationSSA (*this);
				}
			}
		}
	}
	return false;
}

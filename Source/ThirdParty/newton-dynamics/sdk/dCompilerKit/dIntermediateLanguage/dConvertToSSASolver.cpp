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
#include "dCIL.h"
#include "dCILInstr.h"
#include "dBasicBlocksGraph.h"
#include "dCILInstrLoadStore.h"
#include "dConvertToSSASolver.h"



dConvertToSSASolver::dConvertToSSASolver (dBasicBlocksGraph* const graph)
	:m_graph (graph)
{
}



void dConvertToSSASolver::BuildDomicanceFrontier(const dBasicBlock* const root)
{
	dTree<int, const dBasicBlock*> frontier;
	for (dList<const dBasicBlock*>::dListNode* succNode = root->m_successors.GetFirst(); succNode; succNode = succNode->GetNext()) {
		const dBasicBlock& succBlock = *succNode->GetInfo();
		if (succBlock.m_idom != root) {
			frontier.Insert(0, &succBlock);
		}
		//succBlock.Trace();
	}

	for (dList<const dBasicBlock*>::dListNode* node = root->m_children.GetFirst(); node; node = node->GetNext()) {
		const dBasicBlock* const childBlockOuter = node->GetInfo();
		BuildDomicanceFrontier(childBlockOuter);
		
		dAssert (m_dominanceFrontier.Find(childBlockOuter));
		dFrontierList& childFrontierList = m_dominanceFrontier.Find(root)->GetInfo();
		for (dList<const dBasicBlock*>::dListNode* childFrontierNode = childFrontierList.GetFirst(); childFrontierNode; childFrontierNode = childFrontierNode->GetNext()) {
			const dBasicBlock* const childBlock = childFrontierNode->GetInfo();
			if (childBlock->m_idom != root) {
				frontier.Insert(0, childBlock);
			}
		}
	}

	dAssert (m_dominanceFrontier.Find(root));
	dFrontierList& frontierList = m_dominanceFrontier.Find(root)->GetInfo();
	dTree<int, const dBasicBlock*>::Iterator iter(frontier);
	for (iter.Begin(); iter; iter++) {
		frontierList.Append(iter.GetKey());
	}
}


void dConvertToSSASolver::RenameVariables(const dBasicBlock* const root, dTree <dStatementBucket, dString>& stack) const
{
	//root->Trace();
static int xxx;
	bool terminate = false;
	for (dCIL::dListNode* node = root->m_begin; !terminate; node = node->GetNext()) {
		terminate = (node == root->m_end);
		dCILInstr* const instruction = node->GetInfo();
instruction->Trace();
		if (!instruction->GetAsPhi()) {
			dList<dCILInstr::dArg*> variablesList;
xxx++;
			instruction->GetUsedVariables(variablesList);
			for (dList<dCILInstr::dArg*>::dListNode* argNode = variablesList.GetFirst(); argNode; argNode = argNode->GetNext()) {
				dCILInstr::dArg* const variable = argNode->GetInfo();
				dString name(instruction->RemoveSSAPostfix(variable->m_label));
				dAssert(stack.Find(name));
				dStatementBucket& topStack = stack.Find(name)->GetInfo();
				dAssert (topStack.m_stack.GetCount());
				int stackLevel = topStack.m_stack.GetLast()->GetInfo();
				variable->m_label = instruction->MakeSSAName(name, stackLevel);
			}
		}

		dCILInstr::dArg* const variable = instruction->GetGeneratedVariable();
		if (variable) {
			dString name(instruction->RemoveSSAPostfix(variable->m_label));
			dAssert(stack.Find(name));
			dStatementBucket& topStack = stack.Find(name)->GetInfo();

			int i = topStack.m_index;
			variable->m_label = instruction->MakeSSAName(name, i);
			topStack.m_stack.Append(i);
			topStack.m_index = i + 1;
		}
//instruction->Trace();
	}

	for (dList<const dBasicBlock*>::dListNode* node = root->m_successors.GetFirst(); node; node = node->GetNext()) {
		const dBasicBlock* successor = node->GetInfo();
		bool terminate1 = false;
		for (dCIL::dListNode* node1 = successor->m_begin; !terminate1; node1 = node1->GetNext()) {
			terminate1 = (node1 == successor->m_end);
			dCILInstr* const instruction1 = node1->GetInfo();
//instruction1->Trace();
			if (instruction1->GetAsPhi()) {
				dCILInstrPhy* const phyInstruction = instruction1->GetAsPhi();

				dCILInstr::dArg* const variable = phyInstruction->GetGeneratedVariable();
				dString name (phyInstruction->RemoveSSAPostfix(variable->m_label));
				for (dList<dCILInstrPhy::dArgPair>::dListNode* operandNode = phyInstruction->m_sources.GetFirst(); operandNode; operandNode = operandNode->GetNext()) {
					dCILInstrPhy::dArgPair& pair = operandNode->GetInfo(); 
					if (pair.m_block == root) {
						dAssert(stack.Find(name));
						dStatementBucket& topStack = stack.Find(name)->GetInfo();
						int stackLevel = topStack.m_stack.GetLast() ? topStack.m_stack.GetLast()->GetInfo() : 0;
						pair.m_arg.m_label = phyInstruction->MakeSSAName(name, stackLevel);
//instruction1->Trace();
						break;
					}
				}

			}
		}
	}


	for (dList<const dBasicBlock*>::dListNode* node = root->m_children.GetFirst(); node; node = node->GetNext()) {
		RenameVariables(node->GetInfo(), stack);
	}

	for (dCIL::dListNode* node = root->m_end; node != root->m_begin; node = node->GetPrev()) {
		dCILInstr* const instruction = node->GetInfo();
		const dCILInstr::dArg* const variable = instruction->GetGeneratedVariable();
		if (variable) {
			dString name(instruction->RemoveSSAPostfix(variable->m_label));
			dStatementBucket& topStack = stack.Find(name)->GetInfo();
			dAssert(topStack.m_stack.GetLast());
			topStack.m_stack.Remove(topStack.m_stack.GetLast());
		}
	}
}

void dConvertToSSASolver::BuildDomicanceFrontier()
{
	m_dominanceFrontier.RemoveAll();
	for (dBasicBlocksGraph::dListNode* node = m_graph->GetFirst(); node; node = node->GetNext()) {
		m_dominanceFrontier.Insert(&node->GetInfo());
	}

	BuildDomicanceFrontier(&m_graph->GetFirst()->GetInfo());
}

void dConvertToSSASolver::Solve()
{
//m_graph->Trace();
	BuildDomicanceFrontier();
//m_graph->Trace();
	dTree <dStatementBucket, dString> variableList;
	for (dBasicBlocksGraph::dListNode* nodeOuter = m_graph->GetFirst(); nodeOuter; nodeOuter = nodeOuter->GetNext()) {
		dBasicBlock& block = nodeOuter->GetInfo();

		for (dCIL::dListNode* node = block.m_end; node != block.m_begin; node = node->GetPrev()) {
			dCILInstr* const instruction = node->GetInfo();
			//instruction->Trace();
			const dCILInstr::dArg* const variable = instruction->GetGeneratedVariable();
			if (variable) {
				dTree <dStatementBucket, dString>::dTreeNode* entry = variableList.Find(variable->m_label);
				if (!entry) {
					entry = variableList.Insert(variable->m_label);
					entry->GetInfo().m_variable = dCILInstr::dArg(variable->m_label, variable->GetType());
				}
				dStatementBucket& buckect = entry->GetInfo();
				buckect.Insert(instruction, &block);
			}
		}
	}

//m_graph->Trace();

	dCIL* const cil = m_graph->m_begin->GetInfo()->GetCil();
	dTree <dStatementBucket, dString>::Iterator iter (variableList);

	dTree <dStatementBucket, dString> phiVariables;
	for (iter.Begin(); iter; iter ++) {
		dStatementBucket w;
		dStatementBucket::Iterator bucketIter (iter.GetNode()->GetInfo());
		for (bucketIter.Begin(); bucketIter; bucketIter ++) {
			w.Insert(bucketIter.GetNode()->GetInfo(), bucketIter.GetKey());
		}

		//const dString& name = iter.GetKey();
		dCILInstr::dArg name (iter.GetNode()->GetInfo().m_variable);
//dTrace (("\n%s\n", name.m_label.GetStr()));

		dStatementBucket& wPhi = phiVariables.Insert(name.m_label)->GetInfo();
		while (w.GetCount()) {
			const dBasicBlock* const block = w.GetRoot()->GetKey();
			const dCILInstr* const instruction = w.GetRoot()->GetInfo();
			w.Remove(w.GetRoot());

//instruction->Trace();
//block->Trace();
			dAssert(m_dominanceFrontier.Find(block));
			dFrontierList& frontierList = m_dominanceFrontier.Find(block)->GetInfo();
			for (dList<const dBasicBlock*>::dListNode* node = frontierList.GetFirst(); node; node = node->GetNext()) {
				const dBasicBlock* const frontier = node->GetInfo();
				if (!wPhi.Find(frontier)) {

					dList<const dBasicBlock*> sources;
					for (dList<const dBasicBlock*>::dListNode* predNode = frontier->m_predecessors.GetFirst(); predNode; predNode = predNode->GetNext()) {
						const dBasicBlock* const predBlock = predNode->GetInfo();
						sources.Append(predBlock);
					}

					dAssert (sources.GetCount());
					dCILInstrPhy* const phyInstruction = new dCILInstrPhy (*cil, name.m_label, name.GetType(), frontier, sources);

					dCIL::dListNode* phiNode = frontier->m_begin;
					while (phiNode->GetNext()->GetInfo()->GetAsPhi()) {
						phiNode = phiNode->GetNext();
					}
					cil->InsertAfter(phiNode, phyInstruction->GetNode());
					wPhi.Insert(instruction, frontier);
					if (!iter.GetNode()->GetInfo().Find(frontier)) {
						w.Insert(instruction, frontier);
					}
				}
			}
		}
	}

//m_graph->Trace();

	variableList.RemoveAll ();
	for (dBasicBlocksGraph::dListNode* nodeOuter = m_graph->GetFirst(); nodeOuter; nodeOuter = nodeOuter->GetNext()) {
		dBasicBlock& block = nodeOuter->GetInfo();

		for (dCIL::dListNode* node = block.m_end; node != block.m_begin; node = node->GetPrev()) {
			dCILInstr* const instruction = node->GetInfo();
			//instruction->Trace();
			const dCILInstr::dArg* const variable = instruction->GetGeneratedVariable();
			if (variable) {
				dTree <dStatementBucket, dString>::dTreeNode* entry = variableList.Find(variable->m_label);
				if (!entry) {
					entry = variableList.Insert(variable->m_label);
					entry->GetInfo().m_variable = dCILInstr::dArg(variable->m_label, variable->GetType());
				}
				dStatementBucket& buckect = entry->GetInfo();
				buckect.Insert(instruction, &block);
			}
		}
	}
//m_graph->Trace();
	RenameVariables (&m_graph->GetFirst()->GetInfo(), variableList);
//m_graph->Trace();
	m_graph->ApplyDeadCodeEliminationSSA ();
//m_graph->Trace();
}

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
#include "dConvertToSSASolver.h"
#include "dRegisterInterferenceGraph.h"
#include "dConditionalConstantPropagationSolver.h"


void dStatementBlockDictionary::BuildUsedVariableWorklist(dBasicBlocksGraph& graph)
{
	RemoveAll();

	for (dBasicBlocksGraph::dListNode* nodeOuter = graph.GetFirst(); nodeOuter; nodeOuter = nodeOuter->GetNext()) {
		dBasicBlock& block = nodeOuter->GetInfo();

		for (dCIL::dListNode* node = block.m_end; node != block.m_begin; node = node->GetPrev()) {
			dCILInstr* const instruction = node->GetInfo();
			dList<dCILInstr::dArg*> variablesList;
			instruction->GetUsedVariables(variablesList);
			for (dList<dCILInstr::dArg*>::dListNode* varNode = variablesList.GetFirst(); varNode; varNode = varNode->GetNext()) {
				//instruction->Trace();
				const dCILInstr::dArg* const variable = varNode->GetInfo();
				dTreeNode* entry = Find(variable->m_label);
				if (!entry) {
					entry = Insert(variable->m_label);
				}
				dStatementBlockBucket& buckect = entry->GetInfo();
				buckect.Insert(&block, node);
			}
		}
	}
}

dBasicBlock::dBasicBlock (dCIL::dListNode* const begin)
	:m_begin (begin)
	,m_end(NULL)
	,m_idom(NULL)
	,m_children()
	,m_dominators()
	,m_successors()
	,m_predecessors()
	,m_mark(0)
{
}

dBasicBlock::dBasicBlock(const dBasicBlock& src)
	:m_begin(src.m_begin)
	,m_end(NULL)
	,m_idom(NULL)
	,m_children()
	,m_dominators()
	,m_successors()
	,m_predecessors()
	,m_mark(0)
{
	for (m_end = m_begin; m_end && !m_end->GetInfo()->IsBasicBlockEnd(); m_end = m_end->GetNext()) {
		m_end->GetInfo()->m_basicBlock = this;
	}
	m_end->GetInfo()->m_basicBlock = this;
}


void dBasicBlock::Trace() const
{
#if 1
	bool terminate = false;
	for (dCIL::dListNode* node = m_begin; !terminate; node = node->GetNext()) {
		terminate = (node == m_end);
		node->GetInfo()->Trace();
	}
#endif

#if 0
	dCILInstrLabel* const labelOuter = m_begin->GetInfo()->GetAsLabel();
	dTrace ((" block-> %s\n", labelOuter->GetLabel().GetStr()));

	if (m_idom) {
		dTrace(("   immediateDominator-> %s\n", m_idom->m_begin->GetInfo()->GetAsLabel()->GetLabel().GetStr()));
	} else {
		dTrace(("   immediateDominator->\n"));
	}

	dTrace(("   dominatorTreeChildren-> "));
	for (dList<const dBasicBlock*>::dListNode* childNode = m_children.GetFirst(); childNode; childNode = childNode->GetNext()) {
		const dBasicBlock* const childBlock = childNode->GetInfo();
		dCILInstrLabel* const label = childBlock->m_begin->GetInfo()->GetAsLabel();
		dTrace(("%s ", label->GetLabel().GetStr()));
	}
	dTrace(("\n"));

	dTrace (("   dominators-> "));
	dTree<int, const dBasicBlock*>::Iterator iter (m_dominators);
	for (iter.Begin(); iter; iter ++) {
		const dBasicBlock& domBlock = *iter.GetKey();
		dCILInstrLabel* const label = domBlock.m_begin->GetInfo()->GetAsLabel();
		dTrace (("%s ", label->GetLabel().GetStr()));
	}
	dTrace (("\n"));
#endif

/*
	dTrace(("   dominanceFrontier-> "));
	for (dList<const dBasicBlock*>::dListNode* childFrontierNode = m_dominanceFrontier.GetFirst(); childFrontierNode; childFrontierNode = childFrontierNode->GetNext()) {
		const dBasicBlock* const childBlock = childFrontierNode->GetInfo();
		dCILInstrLabel* const label = childBlock->m_begin->GetInfo()->GetAsLabel();
		dTrace(("%s ", label->GetArg0().m_label.GetStr()));
	}
*/
	dTrace (("\n"));
}

bool dBasicBlock::ComparedDominator(const dTree<int, const dBasicBlock*>& newdominators) const
{
	if (m_dominators.GetCount() != newdominators.GetCount()) {
		return true;
	}

	dTree<int, const dBasicBlock*>::Iterator iter0 (m_dominators);
	dTree<int, const dBasicBlock*>::Iterator iter1 (newdominators);
	for (iter0.Begin(), iter1.Begin(); iter0 && iter1; iter0++, iter1++) {
		if (iter0.GetKey() != iter1.GetKey()) {
			return true;
		}
	}
	return false;
}

void dBasicBlock::ReplaceDominator (const dTree<int, const dBasicBlock*>& newdominators)
{
	m_dominators.RemoveAll();
	dTree<int, const dBasicBlock*>::Iterator iter1(newdominators);
	for (iter1.Begin(); iter1; iter1++) {
		m_dominators.Insert(0, iter1.GetKey());
	}
}


dLiveInLiveOutSolver::dLiveInLiveOutSolver(dBasicBlocksGraph* const graph)
	:m_graph(graph)
{
	dList<const dBasicBlock*> reverseOrderBlocks;
	m_graph->BuildReverseOrderBlockList (reverseOrderBlocks);

	dTree<dListNode*, const dBasicBlock*> lastStatement;
	dTree<dListNode*, const dBasicBlock*> firstStatement;
	for (dList<const dBasicBlock*>::dListNode* blockNode = reverseOrderBlocks.GetFirst(); blockNode; blockNode = blockNode->GetNext()) {
		const dBasicBlock* const block = blockNode->GetInfo();

		dListNode* lastNode = GetFirst();
		for (dCIL::dListNode* instNode = block->m_end; instNode != block->m_begin; instNode = instNode->GetPrev()) {
			dCILInstr* const instruction = instNode->GetInfo();
			if (instruction->IsDefineOrUsedVariable()) {
				dFlowGraphNode& entry = Addtop()->GetInfo();
				entry.m_instruction = instruction;
			}
		}
		if (lastNode == GetFirst()) {
			dFlowGraphNode& entry = Addtop()->GetInfo();
			entry.m_instruction = block->m_begin->GetInfo();
		}
		if (!lastNode) {
			lastNode = GetLast();
		} else {
			lastNode = lastNode->GetPrev();
		}
		lastStatement.Insert(lastNode, block);
		firstStatement.Insert(GetFirst(), block);
		if (GetFirst() != lastNode) {
			for (dListNode* node = GetFirst(); node != lastNode; node = node->GetNext()) {
				dFlowGraphNode& point = node->GetInfo();
				point.m_successors.Append(&node->GetNext()->GetInfo());
			}
		}
	}

	for (dList<const dBasicBlock*>::dListNode* blockNode = reverseOrderBlocks.GetFirst(); blockNode; blockNode = blockNode->GetNext()) {
		const dBasicBlock* const block = blockNode->GetInfo();
		dAssert (lastStatement.Find(block));
		dListNode* const lastInstruction = lastStatement.Find(block)->GetInfo();
		for (dList<const dBasicBlock*>::dListNode* successorsNode = block->m_successors.GetFirst(); successorsNode; successorsNode = successorsNode->GetNext()) {
			const dBasicBlock* const succesorBlock = successorsNode->GetInfo();
			dAssert (firstStatement.Find(succesorBlock));
			dListNode* const succesorInstruction = firstStatement.Find(succesorBlock)->GetInfo();
			lastInstruction->GetInfo().m_successors.Append(&succesorInstruction->GetInfo());
		}
	}

	bool someSetChanged = false;
	while (!someSetChanged) {
		someSetChanged = true;
		for (dListNode* pointNode = GetLast(); pointNode; pointNode = pointNode->GetPrev()) {
			dFlowGraphNode& point = pointNode->GetInfo();
			dVariableSet<dString> oldInput(point.m_livedInputSet);
			dVariableSet<dString> oldOutput(point.m_livedOutputSet);

			point.m_livedInputSet.RemoveAll();
			point.m_livedInputSet.Union(point.m_livedOutputSet);
			dCILInstr::dArg* const generatedVar = point.m_instruction->GetGeneratedVariable();
			if (generatedVar) {
				point.m_livedInputSet.Remove(generatedVar->m_label);
			}
			dList<dCILInstr::dArg*> usedVariables;
			point.m_instruction->GetUsedVariables(usedVariables);
			point.m_livedInputSet.Union(usedVariables);
			point.m_livedOutputSet.RemoveAll();

			for (dList<dFlowGraphNode*>::dListNode* successorNode = point.m_successors.GetFirst(); successorNode; successorNode = successorNode->GetNext()) {
				dFlowGraphNode* const successorInfo = successorNode->GetInfo();
				point.m_livedOutputSet.Union(successorInfo->m_livedInputSet);
			}
			someSetChanged = (someSetChanged && oldOutput.Compare(point.m_livedOutputSet) && oldInput.Compare(point.m_livedInputSet));
		}
	}

//	Trace();
}

void dLiveInLiveOutSolver::Trace()
{
	for (dListNode* pointNode = GetFirst(); pointNode; pointNode = pointNode->GetNext()) {
		dFlowGraphNode& point = pointNode->GetInfo();
		dTrace (("liveIn: "));
		point.m_livedInputSet.Trace();
		point.m_instruction->Trace();
		dTrace (("liveOut: "));
		point.m_livedOutputSet.Trace();
		dTrace (("\n"));
	}
}


dBasicBlocksGraph::dBasicBlocksGraph()
	:dBasicBlocksList()
	,m_dominatorTree(NULL)
	,m_mark(0)
	,m_savedRegistersMask(0)
{
}


void dBasicBlocksGraph::Trace() const
{
	for (dList<dBasicBlock>::dListNode* blockNode = GetFirst(); blockNode; blockNode = blockNode->GetNext()) {
		dBasicBlock& block = blockNode->GetInfo();
		block.Trace();
	}
}

void dBasicBlocksGraph::Build (dCIL::dListNode* const functionNode)
{
	dAssert (!GetCount());
	m_begin = functionNode->GetNext();
//	dCIL* const cil = m_begin->GetInfo()->GetCil();
//cil->Trace();
	dAssert (m_begin->GetInfo()->GetAsLabel());
	dTree<int, dCIL::dListNode*> labels;
	for (dCIL::dListNode* node = m_begin->GetNext(); !node->GetInfo()->GetAsFunctionEnd(); node = node->GetNext())
	{
		if (node->GetInfo()->GetAsLabel()) {
			labels.Insert(0, node);
		}
	}

	for (dCIL::dListNode* node = m_begin->GetNext(); !node->GetInfo()->GetAsFunctionEnd(); node = node->GetNext())
	{
		if (node->GetInfo()->GetAsGoto()) {
			dTree<int, dCIL::dListNode*>::dTreeNode* const targetNode = labels.Find(node->GetInfo()->GetAsGoto()->GetTarget());
			dAssert(targetNode);
			targetNode->GetInfo() = 1;
		} else if (node->GetInfo()->GetAsIF()) {
			dTree<int, dCIL::dListNode*>::dTreeNode* const targetNode0 = labels.Find(node->GetInfo()->GetAsIF()->GetTrueTarget());
			dAssert(targetNode0);
			targetNode0->GetInfo() = 1;
			dTree<int, dCIL::dListNode*>::dTreeNode* const targetNode1 = labels.Find(node->GetInfo()->GetAsIF()->GetFalseTarget());
			dAssert(targetNode1);
			targetNode1->GetInfo() = 1;
		}
	}

	dTree<int, dCIL::dListNode*>::Iterator iter (labels);
	for (iter.Begin(); iter; iter++) {
		dTree<int, dCIL::dListNode*>::dTreeNode* node = iter.GetNode();
		if (node->GetInfo() == 0) {
			dCIL::dListNode* const intrNode = iter.GetKey();
			delete intrNode->GetInfo();
		}
	}

	for (m_end = functionNode->GetNext(); !m_end->GetInfo()->GetAsFunctionEnd(); m_end = m_end->GetNext());

	dCIL::dListNode* nextNode;
	for (dCIL::dListNode* node = m_begin; node && !node->GetInfo()->GetAsFunctionEnd(); node = nextNode) {
		nextNode = node->GetNext();
		if (node->GetInfo()->GetAsNop()) {
			delete node->GetInfo();
		}
	}

	for (bool change = true; change; ) {
		change = false;

		for (dCIL::dListNode* node = m_begin; node && node != m_end; node = node->GetNext()) {
			dCILInstr* const instruction = node->GetInfo();
			if (instruction->GetAsGoto()) {
				while (!node->GetNext()->GetInfo()->GetAsLabel()) {
					change = true;
					//cil->Remove(node->GetNext());
					delete node->GetNext()->GetInfo();
				}
			}
		}

		dTree<int, dString> filter;
		for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
			dCILInstr* const instruction = node->GetInfo();
			if (instruction->GetAsGoto()) {
				filter.Insert (0, instruction->GetAsGoto()->GetLabel());
			} else if (instruction->GetAsIF()) {
				filter.Insert (0, instruction->GetAsIF()->GetTrueLabel());
				filter.Insert (0, instruction->GetAsIF()->GetFalseLabel());
			}
		}

		for (dCIL::dListNode* node = m_begin->GetNext(); node != m_end; node = node->GetNext()) {
			dCILInstrLabel* const label = node->GetInfo()->GetAsLabel();
			if (label && !filter.Find (label->GetLabel())) {
//label->Trace();
				
				if (node->GetPrev()->GetInfo()->GetAsLabel()) {
					change = true;
					dCIL::dListNode* const saveNode = node->GetPrev();
					//cil->Remove(node);
					delete (node->GetInfo());
					node = saveNode;

				} else if (node->GetPrev()->GetInfo()->GetAsGoto()) {
					while (!node->GetNext()->GetInfo()->GetAsLabel()) {
						//cil->Remove(node->GetNext());
						delete (node->GetNext()->GetInfo());
					}
					change = true;
					dCIL::dListNode* const saveNode = node->GetPrev();
					//cil->Remove(node);
					delete (node->GetInfo());
					node = saveNode;
				}
			}
		}
	}

	// find the root of all basic blocks leaders
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		if (instruction->IsBasicBlockBegin()) {
			dAssert (instruction->GetAsLabel());
			Append(dBasicBlock(node));
		}
	}
	CalculateSuccessorsAndPredecessors ();
	BuildDominatorTree ();
}

void dBasicBlocksGraph::CalculateSuccessorsAndPredecessors ()
{
	m_mark += 1;
	dList<dBasicBlock*> stack;
	stack.Append(&GetFirst()->GetInfo());

	while (stack.GetCount()) {
		dBasicBlock* const block = stack.GetLast()->GetInfo();
		stack.Remove(stack.GetLast()->GetInfo());
//block->Trace();

		if (block->m_mark < m_mark) {
			block->m_mark = m_mark;
			dCILInstr* const instruction = block->m_end->GetInfo();
//instruction->Trace();
			dAssert(instruction->IsBasicBlockEnd());
			if (instruction->GetAsIF()) {
				dCILInstrConditional* const ifInstr = instruction->GetAsIF();

				dAssert (ifInstr->GetTrueTarget());
				dAssert (ifInstr->GetFalseTarget());

				dCILInstrLabel* const target0 = ifInstr->GetTrueTarget()->GetInfo()->GetAsLabel();
				dCILInstrLabel* const target1 = ifInstr->GetFalseTarget()->GetInfo()->GetAsLabel();

				dBasicBlock* const block0 = target0->m_basicBlock;
				dAssert (block0);
				block->m_successors.Append (block0);
				block0->m_predecessors.Append(block);
				stack.Append (block0);

				dBasicBlock* const block1 = target1->m_basicBlock;
				dAssert(block1);
				block->m_successors.Append(block1);
				block1->m_predecessors.Append(block);
				stack.Append(block1);

			} else if (instruction->GetAsGoto()) {
				dCILInstrGoto* const gotoInst = instruction->GetAsGoto();

				dAssert(gotoInst->GetTarget());
				dCILInstrLabel* const target = gotoInst->GetTarget()->GetInfo()->GetAsLabel();
				dBasicBlock* const block0 = target->m_basicBlock;

				dAssert(block0);
				block->m_successors.Append(block0);
				block0->m_predecessors.Append(block);
				stack.Append(block0);
			}
		}
	}

	DeleteUnreachedBlocks();
}

void dBasicBlocksGraph::DeleteUnreachedBlocks()
{
	dTree<dBasicBlock*, dCIL::dListNode*> blockMap;
	for (dBasicBlocksGraph::dListNode* blockNode = GetFirst(); blockNode; blockNode = blockNode->GetNext()) {
		dBasicBlock& block = blockNode->GetInfo();
		blockMap.Insert(&block, block.m_begin);
	}

	
	m_mark += 1;
	dList<dBasicBlock*> stack;
	stack.Append(&GetFirst()->GetInfo());
	while (stack.GetCount()) {
		dBasicBlock* const block = stack.GetLast()->GetInfo();

		stack.Remove(stack.GetLast()->GetInfo());
		if (block->m_mark < m_mark) {
			block->m_mark = m_mark;

			dCILInstr* const instruction = block->m_end->GetInfo();
			dAssert(instruction->IsBasicBlockEnd());
			if (instruction->GetAsIF()) {
				dCILInstrConditional* const ifInstr = instruction->GetAsIF();
				stack.Append(blockMap.Find(ifInstr->GetTrueTarget())->GetInfo());
				stack.Append(blockMap.Find(ifInstr->GetFalseTarget())->GetInfo());

			} else if (instruction->GetAsGoto()) {
				dCILInstrGoto* const gotoInst = instruction->GetAsGoto();
				stack.Append(blockMap.Find(gotoInst->GetTarget())->GetInfo());
			}
		}
	}

	dBasicBlocksGraph::dListNode* nextBlockNode;
	for (dBasicBlocksGraph::dListNode* blockNode = GetFirst(); blockNode; blockNode = nextBlockNode) {
		dBasicBlock& block = blockNode->GetInfo();
		nextBlockNode = blockNode->GetNext();
		if (block.m_mark != m_mark) {
			bool terminate = false;
			dCIL::dListNode* nextNode;
			for (dCIL::dListNode* node = block.m_begin; !terminate; node = nextNode) {
				terminate = (node == block.m_end);
				nextNode = node->GetNext();
				//cil->Remove(node);
				delete (node->GetInfo());
			}
			Remove(blockNode);
		}
	}
}

void dBasicBlocksGraph::BuildDominatorTree ()
{
	// dominator of the start node is the start itself
	//Dom(n0) = { n0 }
	dBasicBlock& firstBlock = GetFirst()->GetInfo();
	firstBlock.m_dominators.Insert(0, &firstBlock);

	// for all other nodes, set all nodes as the dominators
	for (dListNode* node = GetFirst()->GetNext(); node; node = node->GetNext()) {
		dBasicBlock& block = node->GetInfo();
		for (dListNode* node1 = GetFirst(); node1; node1 = node1->GetNext()) {
			block.m_dominators.Insert(0, &node1->GetInfo());
		}
	}

	bool change = true;
	while (change) {
		change = false;
		for (dListNode* node = GetFirst()->GetNext(); node; node = node->GetNext()) {
			dBasicBlock& blockOuter = node->GetInfo();

//block.Trace();
			dTree<int, const dBasicBlock*> predIntersection;
			const dBasicBlock& predBlockOuter = *blockOuter.m_predecessors.GetFirst()->GetInfo();

			dTree<int, const dBasicBlock*>::Iterator domIter (predBlockOuter.m_dominators);
			for (domIter.Begin(); domIter; domIter ++) {
				const dBasicBlock* const block = domIter.GetKey();
				predIntersection.Insert (0, block);
			}

			for (dList<const dBasicBlock*>::dListNode* predNode = blockOuter.m_predecessors.GetFirst()->GetNext(); predNode; predNode = predNode->GetNext()) {
				const dBasicBlock& predBlock = *predNode->GetInfo();
				dTree<int, const dBasicBlock*>::Iterator predIter (predIntersection);
				for (predIter.Begin(); predIter; ) {
					const dBasicBlock* block = predIter.GetKey();
					predIter ++;
					if (!predBlock.m_dominators.Find(block)) {
						predIntersection.Remove(block);
					}
				}
			}

			dAssert (!predIntersection.Find(&blockOuter));
			predIntersection.Insert(&blockOuter);

			bool dominatorChanged = blockOuter.ComparedDominator (predIntersection);
			if (dominatorChanged) {
				blockOuter.ReplaceDominator (predIntersection);
				//block.Trace();
			}
			change |= dominatorChanged;
		}
	}

	// find the immediate dominator of each block
	for (dListNode* node = GetFirst()->GetNext(); node; node = node->GetNext()) {
		dBasicBlock& block = node->GetInfo();
//block.Trace();

		dAssert (block.m_dominators.GetCount() >= 2);
		dTree<int, const dBasicBlock*>::Iterator iter(block.m_dominators);

		for (iter.Begin(); iter; iter, iter++) {
			const dBasicBlock* const sblock = iter.GetKey();
			if (sblock != &block) {
				if (sblock->m_dominators.GetCount() == (block.m_dominators.GetCount() - 1)) {
					bool identical = true;
					dTree<int, const dBasicBlock*>::Iterator iter0(block.m_dominators);
					dTree<int, const dBasicBlock*>::Iterator iter1(sblock->m_dominators);
					for (iter0.Begin(), iter1.Begin(); identical && iter0 && iter1; iter0++, iter1++) {
						if (iter0.GetKey() == &block) {
							iter0 ++;
						}
						identical &= (iter0.GetKey() == iter1.GetKey());
					}
					if (identical) {
						if (sblock->m_dominators.GetCount() == (block.m_dominators.GetCount() - 1)) {
							block.m_idom = sblock;
							break;
						}
					}
				}
			}
		}
		dAssert (block.m_idom);
		//block.m_idom->Trace();
	}

//Trace();
	// build dominator tree
	m_dominatorTree = &GetFirst()->GetInfo();
	for (dListNode* node = GetFirst()->GetNext(); node; node = node->GetNext()) {
		const dBasicBlock& block = node->GetInfo();
		block.m_idom->m_children.Append(&block);
	}
	
//Trace();
}

void dBasicBlocksGraph::ConvertToSSA ()
{
	dConvertToSSASolver ssa (this);
	ssa.Solve();
}

void dBasicBlocksGraph::GetStatementsWorklist(dWorkList& workList) const
{
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		workList.Insert(node->GetInfo());
	}
}

void dBasicBlocksGraph::BuildReverseOrderBlockList(dList<const dBasicBlock*>& reverseOrderBlocks)
{
	for (dBasicBlocksList::dListNode* blockNode = GetFirst(); blockNode; blockNode = blockNode->GetNext()) {
		const dBasicBlock& block = blockNode->GetInfo();
		block.m_mark = m_mark;
	}
	m_mark++;
	BuildReverseOrdeBlockList(reverseOrderBlocks, &GetFirst()->GetInfo());
}

void dBasicBlocksGraph::BuildReverseOrdeBlockList(dList<const dBasicBlock*>& reverseOrderList, const dBasicBlock* const block) const
{
	if (block->m_mark != m_mark) {
		block->m_mark = m_mark;
		for (dList<const dBasicBlock*>::dListNode* successorNode = block->m_successors.GetFirst(); successorNode; successorNode = successorNode->GetNext()) {
			BuildReverseOrdeBlockList(reverseOrderList, successorNode->GetInfo());
		}
		reverseOrderList.Append(block);
	}
}


bool dBasicBlocksGraph::ApplyDeadCodeEliminationSSA()
{
	bool anyChanges = false;

	dWorkList workList;
	dStatementBlockDictionary usedVariablesList;
	usedVariablesList.BuildUsedVariableWorklist(*this);

	dTree<dCIL::dListNode*, dString> map;
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		const dCILInstr::dArg* const variable = instruction->GetGeneratedVariable();
		if (variable) {
			map.Insert(node, variable->m_label);
		}
	}

	GetStatementsWorklist(workList);
	while (workList.GetCount()) {
		dCIL::dListNode* const node = workList.GetRoot()->GetInfo();
		workList.Remove(workList.GetRoot());
		dCILInstr* const instruction = node->GetInfo();
		//instruction->Trace();
		if (!instruction->GetAsCall()) {
			const dCILInstr::dArg* const variableOuter = instruction->GetGeneratedVariable();
			if (variableOuter) {
				dStatementBlockDictionary::dTreeNode* const usesNodeBuckect = usedVariablesList.Find(variableOuter->m_label);
				dAssert(!usesNodeBuckect || usesNodeBuckect->GetInfo().GetCount());
				if (!usesNodeBuckect) {
					anyChanges = true;
					dList<dCILInstr::dArg*> variablesList;
					instruction->GetUsedVariables(variablesList);
					for (dList<dCILInstr::dArg*>::dListNode* varNode = variablesList.GetFirst(); varNode; varNode = varNode->GetNext()) {
						const dCILInstr::dArg* const variable = varNode->GetInfo();
						dAssert(usedVariablesList.Find(variable->m_label));
						dStatementBlockDictionary::dTreeNode* const entry = usedVariablesList.Find(variable->m_label);
						if (entry) {
							dStatementBlockBucket& buckect = entry->GetInfo();
							buckect.Remove(node);
							dAssert(map.Find(variable->m_label) || instruction->GetAsPhi());
							if (map.Find(variable->m_label)) {
								workList.Insert(map.Find(variable->m_label)->GetInfo()->GetInfo());
								if (!buckect.GetCount()) {
									usedVariablesList.Remove(entry);
								}
							}
						}
					}
					instruction->Nullify();
				}
			}
		}
	}
	return anyChanges;
}


bool dBasicBlocksGraph::ApplyCopyPropagationSSA()
{
	bool anyChanges = false;

	dWorkList workList;
	dStatementBlockDictionary usedVariablesList;
	usedVariablesList.BuildUsedVariableWorklist (*this);

	GetStatementsWorklist(workList);
	while (workList.GetCount()) {
		dCIL::dListNode* const node = workList.GetRoot()->GetInfo();
		workList.Remove(workList.GetRoot());
		dCILInstr* const instruction = node->GetInfo();
		anyChanges |= instruction->ApplyCopyPropagationSSA(workList, usedVariablesList);
	}
	return anyChanges;
}

bool dBasicBlocksGraph::ApplySimpleConstantPropagationSSA()
{
	bool anyChanges = false;

	dWorkList workList;
	dStatementBlockDictionary usedVariablesList;
	usedVariablesList.BuildUsedVariableWorklist(*this);

	GetStatementsWorklist(workList);
	while (workList.GetCount()) {
		dCIL::dListNode* const node = workList.GetRoot()->GetInfo();
		workList.Remove(workList.GetRoot());
		dCILInstr* const instruction = node->GetInfo();
//instruction->Trace();
		bool change = instruction->ApplySimpleConstantPropagationSSA(workList, usedVariablesList);
//instruction->Trace();
		anyChanges |= change;
	}
	return anyChanges;
}

/*
bool dBasicBlocksGraph::ApplyConditionalConstantPropagationSSA()
{
	bool anyChanges = false;

	dTree<int, dCIL::dListNode*> phyMap;
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		if (instruction->GetAsPhi()) {
			phyMap.Insert(0, node);
		}
	}

	dCIL* const cil = m_begin->GetInfo()->GetCil();
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstr* const instruction = node->GetInfo();
		if (instruction->GetAsIF()) {
			dCILInstrConditional* const conditinal = instruction->GetAsIF();

			const dCILInstr::dArg& arg0 = conditinal->GetArg0();
			if ((arg0.GetType().m_intrinsicType == dCILInstr::m_constInt) || (arg0.GetType().m_intrinsicType == dCILInstr::m_constFloat)) {
				dAssert(conditinal->GetTrueTarget());
				dAssert(conditinal->GetFalseTarget());
				dAssert(0);

				int condition = arg0.m_label.ToInteger();
				if (conditinal->m_mode == dCILInstrConditional::m_ifnot) {
					condition = !condition;
				}

				dCILInstrLabel* label;
				if (condition) {
					label = conditinal->GetTrueTarget()->GetInfo()->GetAsLabel();
				} else {
					label = conditinal->GetFalseTarget()->GetInfo()->GetAsLabel();
				}

				//dCILInstrGoto* const jump = new dCILInstrGoto(*cil, label->GetLabel);
				//jump->SetTarget(label);
				dCILInstrGoto* const jump = new dCILInstrGoto(*cil, label);
				conditinal->ReplaceInstruction(jump);
				anyChanges = true;

			}
		}
	}

	return anyChanges;
}
*/

/*
bool dBasicBlocksGraph::ApplyConstantPropagationSSA()
{
	bool anyChanges = false;

	dWorkList workList;
	dStatementBlockDictionary usedVariablesList;
	usedVariablesList.BuildUsedVariableWorklist(*this);

	GetStatementsWorklist(workList);
	while (workList.GetCount()) {
		dCIL::dListNode* const node = workList.GetRoot()->GetKey();
		workList.Remove(workList.GetRoot());
		dCILInstr* const instruction = node->GetInfo();
		anyChanges |= instruction->ApplyConstantPropagationSSA(workList, usedVariablesList);
	}

	if (anyChanges) {
		for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
			dCILInstr* const instruction = node->GetInfo();
			instruction->ApplyConstantFoldingSSA();
		}
	}
	return anyChanges;
}
*/

bool dBasicBlocksGraph::ApplyConditionalConstantPropagationSSA()
{
	dConditionalConstantPropagationSolver constantPropagation(this);
	return false;
	//return constantPropagation.Solve();
}

void dBasicBlocksGraph::InsertCommonSpillsSSA()
{
//Trace();
	
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstrArgument* const firstArg = node->GetInfo()->GetAsArgument();
		if (firstArg) {
			dCIL* const cil = firstArg->m_cil;
			const dCILInstr::dArg& param = firstArg->GetArg0();
			dCILInstr::dArg newArg(cil->NewTemp(), param.GetType());
			for (dCIL::dListNode* node1 = node->GetNext(); node1 && !node1->GetInfo()->GetAsFunctionEnd(); node1 = node1->GetNext()) {
				dCILInstr* const instruction = node1->GetInfo();
				//instruction->Trace();
				instruction->ReplaceArgument(param, newArg);
				//instruction->Trace();
			}
			dCILInstrMove* const moveInstr = new dCILInstrMove(*cil, newArg.m_label, newArg.GetType(), param.m_label, param.GetType());
			moveInstr->m_basicBlock = firstArg->m_basicBlock;
			cil->InsertAfter(firstArg->GetNode(), moveInstr->GetNode());
			break;
		}
	}
//Trace();

	bool hasCalls = false;
	for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
		dCILInstrCall* const functionCall = node->GetInfo()->GetAsCall();
		if (functionCall) {
			hasCalls = true;
			break;
		}
	}

	if (hasCalls) {
		dTree<dCILInstr*, dString> definedWorkList;
		for (dCIL::dListNode* node = m_begin; node != m_end; node = node->GetNext()) {
			dCILInstr* const instruction = node->GetInfo();
			const dCILInstr::dArg* const arg = instruction->GetGeneratedVariable();
			if (arg) {
				definedWorkList.Insert(instruction, arg->m_label);
			}
		}

		dStatementBlockDictionary usedList;
		dLiveInLiveOutSolver liveInLiveOut(this);
		usedList.BuildUsedVariableWorklist(*this);
		for (dLiveInLiveOutSolver::dListNode* node = liveInLiveOut.GetFirst(); node; node = node->GetNext()) {
			dFlowGraphNode& point = node->GetInfo();
			dCILInstrCall* const functionCall = point.m_instruction->GetAsCall();
			if (functionCall) {
				const dCILInstr::dArg* const arg = functionCall->GetGeneratedVariable();
				dVariableSet<dString>::Iterator iter (point.m_livedOutputSet);
				for (iter.Begin(); iter; iter ++) {
					const dString& var = iter.GetKey(); 
					if (var != arg->m_label) {
						dTree<dCILInstr*, dString>::dTreeNode* const definedNode = definedWorkList.Find(var);
						dAssert (definedNode);
						dCILInstr* const definedInstr = definedNode->GetInfo();
						const dCILInstr::dArg* const defArg = definedInstr->GetGeneratedVariable();
						dAssert (defArg->m_label == var);
						
						dCILInstr::dArg newArg (definedInstr->m_cil->NewTemp(), defArg->GetType());
						dCILInstrMove* const moveInstr = new dCILInstrMove (*definedInstr->m_cil, newArg.m_label, newArg.GetType(), defArg->m_label, defArg->GetType());
						moveInstr->m_basicBlock = definedInstr->m_basicBlock;
						definedInstr->m_cil->InsertAfter(definedInstr->GetNode(), moveInstr->GetNode());
						
						dAssert (usedList.Find(var));
						dStatementBlockBucket::Iterator usedIter(usedList.Find(var)->GetInfo());
						for (usedIter.Begin(); usedIter; usedIter ++) {
							const dCIL::dListNode* const instrNode = usedIter.GetKey();
							dCILInstr* const instruction = instrNode->GetInfo();
							//instruction->Trace();
							instruction->ReplaceArgument(*defArg, newArg);
							//instruction->Trace();
						}
					}
				}
			}
		}
	}
	//Trace();
}

void dBasicBlocksGraph::RemovePhiFunctionsSSA()
{
	for (dBasicBlocksGraph::dListNode* nodeOuter = GetFirst(); nodeOuter; nodeOuter = nodeOuter->GetNext()) {
		dBasicBlock& block = nodeOuter->GetInfo();
		if (block.m_predecessors.GetCount() > 1) {
			for (dCIL::dListNode* node = block.m_begin; node != block.m_end; node = node->GetNext()) {
				dCILInstrPhy* const phyInstruction = node->GetInfo()->GetAsPhi();
				if (phyInstruction) {
					dCIL* const cil = phyInstruction->m_cil;
					dAssert(block.m_predecessors.GetCount() == phyInstruction->m_sources.GetCount());

					dList<dCILInstrPhy::dArgPair>::dListNode* pairNode = phyInstruction->m_sources.GetFirst();
					dList<const dBasicBlock*>::dListNode* predecessorsNode = block.m_predecessors.GetFirst();
					for (int i = 0; i < block.m_predecessors.GetCount(); i++) {
						dCILInstrPhy::dArgPair& var = pairNode->GetInfo();
						const dBasicBlock* const predecessor = predecessorsNode->GetInfo();
						dCILInstrMove* const move = new dCILInstrMove(*cil, phyInstruction->GetArg0().m_label, phyInstruction->GetArg0().GetType(), var.m_arg.m_label, var.m_arg.GetType());
						cil->InsertAfter(predecessor->m_end->GetPrev(), move->GetNode());
						pairNode = pairNode->GetNext();
						predecessorsNode = predecessorsNode->GetNext();
					}
					phyInstruction->Nullify();
					//cil->Trace();
				}
			}
		}
	}
//Trace();
}


void dBasicBlocksGraph::RegistersAllocations ()
{
	int regCount = 16;
	InsertCommonSpillsSSA();
	RemovePhiFunctionsSSA();
	dRegisterInterferenceGraph interferenceGraph(this, regCount);
}

void dBasicBlocksGraph::OptimizeSSA()
{
//Trace();
	bool actionFound = true;
	for (int i = 0; actionFound && i < 32; i ++) {
		actionFound = false;
Trace();
		actionFound |= ApplySimpleConstantPropagationSSA();
Trace();
//		actionFound |= ApplyConditionalConstantPropagationSSA();
//Trace();
		actionFound |= ApplyCopyPropagationSSA();
Trace();
		actionFound |= ApplyDeadCodeEliminationSSA();
Trace();
	}
	dAssert (!actionFound);
}




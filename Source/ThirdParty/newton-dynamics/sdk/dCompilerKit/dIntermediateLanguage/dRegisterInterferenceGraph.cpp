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
#include "dCILInstrBranch.h"
#include "dBasicBlocksGraph.h"
#include "dCILInstrLoadStore.h"
#include "dCILInstrMiscellaneous.h"
#include "dRegisterInterferenceGraph.h"


#define D_TRACE_INTERFIRANCE_GRAPH

bool dRegisterInterferenceGraph::IsSpilledVariable (const dString& name) const
{
	static int spillSize = strlen (D_SPILL_REGISTER_SYMBOL);
	return strncmp (name.GetStr(), D_SPILL_REGISTER_SYMBOL, spillSize) ? false : true;
}

dRegisterInterferenceGraph::dRegisterInterferenceGraph (dBasicBlocksGraph* const graph, int registerCount)
	:dTree<dRegisterInterferenceNode, dString>()
	,m_graph(graph)
{
//	m_flowGraph->ApplySemanticInstructionReordering();
//m_graph->Trace();
	Build();
	while (ColorGraph () > registerCount) {
		// we have a spill, find a good spill node and try graph coloring again
		dAssert (0);
	}
//m_graph->Trace();
	AllocateRegisters();
//m_graph->Trace();

	ApplyDeadCodeElimination();
//m_graph->Trace();
	while (RemoveRedundantJumps());
	RemoveNop();
//m_graph->m_begin->GetInfo()->GetCil()->Trace();
}


/*
dRegisterInterferenceGraph::dTreeNode* dRegisterInterferenceGraph::GetBestNode(int& edgeCount)
{
	int minCount = 0x7fffff;
	dTreeNode* bestNode = NULL;
	Iterator iter(*this);
	for (iter.Begin(); iter; iter++) {
		dTreeNode* const node = iter.GetNode();
		dRegisterInterferenceNode& info = node->GetInfo();
		if (!info.m_inSet) {
			int count = 0;
			for (dList<dRegisterInterferenceNodeEdge>::dListNode* edgeNode = info.m_interferanceEdge.GetFirst(); edgeNode && (count < minCount); edgeNode = edgeNode->GetNext()) {
				dRegisterInterferenceNodeEdge& edge = edgeNode->GetInfo();
				if (!edge.m_mark) {
					count++;
				}
			}
			if (count < minCount) {
				minCount = count;
				bestNode = node;
			}
		}
	}

	edgeCount = minCount;
	dRegisterInterferenceNode& info = bestNode->GetInfo();
	info.m_inSet = true;
	return bestNode;
}
*/

dRegisterInterferenceNodeEdge* dRegisterInterferenceNode::FindEdge(const dString& var)
{
	for (dList<dRegisterInterferenceNodeEdge>::dListNode* node = m_interferanceEdge.GetFirst(); node; node = node->GetNext()) {
		dRegisterInterferenceNodeEdge& info = node->GetInfo();
		const dString& edgeVar = info.m_incidentNode->GetKey();
		if (edgeVar == var) {
			return &info;
		}
	}
	return NULL;
}

int dRegisterInterferenceGraph::ColorGraph()
{
#if 0
	dList<dTreeNode*> registerOrder;
	for (int i = 0; i < GetCount(); i++) {
		int edgeCount; 
		dTreeNode* const bestNode = GetBestNode(edgeCount);
dTrace (("%s\n", bestNode->GetKey().GetStr()));
		registerOrder.Addtop(bestNode);
		dRegisterInterferenceNode& node = bestNode->GetInfo();
		for (dList<dRegisterInterferenceNodeEdge>::dListNode* ptr = node.m_interferanceEdge.GetFirst(); ptr; ptr = ptr->GetNext()) {
			dRegisterInterferenceNodeEdge& edge = ptr->GetInfo();
			edge.m_mark = true;
			dRegisterInterferenceNode* const twinNode = &edge.m_incidentNode->GetInfo();
			for (dList<dRegisterInterferenceNodeEdge>::dListNode* twinPtr = twinNode->m_interferanceEdge.GetFirst(); twinPtr; twinPtr = twinPtr->GetNext()) {
				dRegisterInterferenceNodeEdge& twinEdge = twinPtr->GetInfo();
				if (twinEdge.m_incidentNode == bestNode) {
					twinEdge.m_mark = true;
					break;
				}
			}
		}
	}

	int registersUsed = -1;
	for (dList<dTreeNode*>::dListNode* node = registerOrder.GetFirst(); node; node = node->GetNext()) {
		dTreeNode* const varNode = node->GetInfo();
		dRegisterInterferenceNode& variable = varNode->GetInfo();
		if (variable.m_registerIndex == -1) {
			int regMask = 0;
			for (dList<dRegisterInterferenceNodeEdge>::dListNode* ptr = variable.m_interferanceEdge.GetFirst(); ptr; ptr = ptr->GetNext()) {
				dRegisterInterferenceNodeEdge& edge = ptr->GetInfo();
				const dRegisterInterferenceNode& otherVariable = edge.m_incidentNode->GetInfo();
				if (otherVariable.m_registerIndex != -1) {
					regMask |= (1 << otherVariable.m_registerIndex);
				}
			}

			int index = 0;
			if (variable.m_saveRegisterOnEntry) {
				dAssert(0);
				//index = D_CALLER_SAVE_REGISTER_COUNT;
				//const int highMask = variable.m_saveRegisterOnEntry << D_CALLER_SAVE_REGISTER_COUNT;
				//for (int mask = regMask; mask & highMask; mask >>= 1) {
				//	index++;
				//}
				//if (index > (D_INTEGER_REGISTER_COUNT - 1)) {
				//	dAssert(0);
				//	index = 0;
				//	for (int mask = regMask; mask & 1; mask >>= 1) {
				//		index++;
				//	}
				//}
			} else {
				for (int mask = regMask; mask & 1; mask >>= 1) {
					index++;
				}
			}

			if (index > (D_INTEGER_REGISTER_COUNT - 1)) {
				// spill
				dAssert(0);
				return  D_INTEGER_REGISTER_COUNT;
				
			}
			variable.m_registerIndex = index;
		}
		registersUsed = dMax(registersUsed, variable.m_registerIndex);
	}

	// remap register
	int count = 0;
	int registerCost[1024];
	dTreeNode* registerNode[1024];

	m_graph->m_savedRegistersMask = 0;
	dAssert(registerOrder.GetCount() < sizeof(registerCost) / sizeof(registerCost[0]));
	for (dList<dTreeNode*>::dListNode* node = registerOrder.GetFirst(); node; node = node->GetNext()) {
		dTreeNode* const varNode = node->GetInfo();
		dRegisterInterferenceNode& variable = varNode->GetInfo();
		if (variable.m_saveRegisterOnEntry) {
			registerCost[count] = (variable.m_saveRegisterOnEntry << 24) + (variable.m_registerIndex << 1);
		} else {
			registerCost[count] = (variable.m_registerIndex << 1) + !variable.m_isPrecolored;
		}
		registerNode[count] = varNode;
		count++;
	}

	for (int i = 1; i < count; i++) {
		int cost = registerCost[i];
		dTreeNode* const varNode = registerNode[i];
		int j = i - 1;
		for (; (j >= 0) && (registerCost[j] > cost); j--) {
			registerCost[j + 1] = registerCost[j];
			registerNode[j + 1] = registerNode[j];
		}
		registerCost[j + 1] = cost;
		registerNode[j + 1] = varNode;
	}

	int saveOnEntryStart = 100000;
	for (int i = 0; i < count; i++) {
		int cost = registerCost[i];
		if (cost >= 1 << 24) {
			saveOnEntryStart = i;
			break;
		}
	}
#else

	// my algorithm for register allocation from interference graph, seems better than Chaitin method
	// at least is much faster and yielding the same result.
	Iterator iter(*this);
	int registersUsed = -1;
	for (iter.Begin(); iter; iter++) {
		dTreeNode* const node = iter.GetNode();
		dRegisterInterferenceNode& variable = node->GetInfo();
		if (variable.m_registerIndex == -1) {
			//int regMask = 0;
			int regMask = variable.m_registerIndexBase ? (1 << variable.m_registerIndexBase) - 1 : 0;
			for (dList<dRegisterInterferenceNodeEdge>::dListNode* ptr = variable.m_interferanceEdge.GetFirst(); ptr; ptr = ptr->GetNext()) {
				dRegisterInterferenceNodeEdge& edge = ptr->GetInfo();
				const dRegisterInterferenceNode& otherVariable = edge.m_incidentNode->GetInfo();
				if (otherVariable.m_registerIndex != -1) {
					regMask |= (1 << otherVariable.m_registerIndex);
				}
			}

			int index = 0;
			for (int mask = regMask; mask & 1; mask >>= 1) {
				index++;
			}

			if (index > (D_INTEGER_REGISTER_COUNT - 1)) {
				// spill
				dAssert(0);
				return  D_INTEGER_REGISTER_COUNT;
			}
			variable.m_registerIndex = index;
		}
		registersUsed = dMax(registersUsed, variable.m_registerIndex);
	}
#endif

/*
	int count = 0;
	int registerCost[1024];
	dTreeNode* registerNode[1024];

	m_graph->m_savedRegistersMask = 0;
	dAssert(GetCount() < sizeof(registerCost) / sizeof(registerCost[0]));
	for (iter.Begin(); iter; iter++) {
		dTreeNode* const varNode = iter.GetNode();
		dRegisterInterferenceNode& variable = varNode->GetInfo();
		//		if (variable.m_saveRegisterOnEntry____) {
		//			registerCost[count] = (variable.m_saveRegisterOnEntry____ << 24) + (variable.m_registerIndex << 1);
		//		} else {
		//			//registerCost[count] = (variable.m_registerIndex << 1) + !variable.m_isPrecolored;
		//			registerCost[count] = (variable.m_registerIndex << 1);
		//		}
		registerCost[count] = (variable.m_registerIndex << 1);

		registerNode[count] = varNode;
		count++;
	}

//	int saveOnEntryIndex = D_CALLER_SAVE_REGISTER_COUNT;
	int saveOnEntryIndex = 4;
	memset(registerCost, -1, count * sizeof(registerCost[0]));
	for (int i = 0; i < count; i++) {
		dTreeNode* const varNode = registerNode[i];
		dRegisterInterferenceNode& variable = varNode->GetInfo();
		int index = variable.m_registerIndex;
		if (registerCost[index] == -1) {
			//if (variable.m_saveRegisterOnEntry____) {
			//	registerCost[index] = saveOnEntryIndex;
			//	m_graph->m_savedRegistersMask |= (1 << saveOnEntryIndex);
			//	saveOnEntryIndex++;
			//} else if (variable.m_isPrecolored) {
			//	registerCost[index] = variable.m_registerIndex;
			//} else {
			//	registerCost[index] = variable.m_registerIndex;
			//}
			registerCost[index] = variable.m_registerIndex;
		}
		variable.m_registerIndex = registerCost[index];
	}
*/
	return registersUsed + 1;
}


void dRegisterInterferenceGraph::AllocateRegisters()
{
	for (dCIL::dListNode* node = m_graph->m_begin; node != m_graph->m_end; node = node->GetNext()) {
		dCILInstr* const instr = node->GetInfo();
		instr->AssignRegisterName(*this);
	}

	int savedRegMask = 0;
	Iterator iter(*this);
	for (iter.Begin(); iter; iter++) {
		const dRegisterInterferenceNode& node = iter.GetNode()->GetInfo();
		if (node.m_registerIndex >= D_CALLEE_SAVE_REGISTER_START) {
			savedRegMask |= 1 << (node.m_registerIndex - D_CALLEE_SAVE_REGISTER_START);
		}
	}

	if (savedRegMask) {
		dCILInstr* const pushReference = m_graph->m_begin->GetInfo();
		dCIL* const cil = pushReference->GetCil();
		int index = 0;
		for (; savedRegMask & (1 << index); index ++) {
			dString reg (IndexToRegister(index + D_CALLEE_SAVE_REGISTER_START));
			dCILInstrPush* const push = new dCILInstrPush (*cil, reg, dCILInstr::dArgType());
			cil->InsertAfter (pushReference->GetNode(), push->GetNode());
		}

		dCILInstr* const popReference = m_graph->m_end->GetPrev()->GetPrev()->GetInfo();
		for (int i = 0; savedRegMask & (1 << i); i ++) {
			dString reg(IndexToRegister(index - i - 1 + D_CALLEE_SAVE_REGISTER_START));
			dCILInstrPop* const pop = new dCILInstrPop(*cil, reg, dCILInstr::dArgType());
			cil->InsertAfter(popReference->GetNode(), pop->GetNode());
		} 
	}
}


int dRegisterInterferenceGraph::GetRegisterIndex(const dString& varName) const
{
	int index = -1;
	dTreeNode* const node = Find(varName);
	dRegisterInterferenceNode& var = node->GetInfo();
	index = var.m_registerIndex;
	return index;
}

dString dRegisterInterferenceGraph::GetRegisterName(const dString& varName) const
{
	int index = GetRegisterIndex(varName);
	if (index == -1) {
		dAssert(0);
		return varName;
	} else {
		return IndexToRegister(index);
	}
}


void dRegisterInterferenceGraph::Build()
{
//m_graph->Trace();
	dLiveInLiveOutSolver liveInLiveOut(m_graph);
	
	for (dLiveInLiveOutSolver::dListNode* node = liveInLiveOut.GetFirst(); node; node = node->GetNext()) {
		dList<dCILInstr::dArg*> usedVariables;
		dFlowGraphNode& point = node->GetInfo();
//point.m_instruction->Trace();		
		dCILInstr::dArg* const generated = point.m_instruction->GetGeneratedVariable();
		if (generated) {
//dTrace (("%s\n", generated->m_label.GetStr()));
			Insert(generated->m_label);
		}

		point.m_instruction->GetUsedVariables(usedVariables);
		for (dList<dCILInstr::dArg*>::dListNode* usedVarNode = usedVariables.GetFirst(); usedVarNode; usedVarNode = usedVarNode->GetNext()) {	
			const dString& variable = usedVarNode->GetInfo()->m_label;
//dTrace (("%s\n", variable.GetStr()));
			if (IsSpilledVariable(variable)) {
				dAssert (0);
				dTreeNode* interferanceGraphNode = Find(variable);
				if (!interferanceGraphNode) {
					interferanceGraphNode = Insert(variable);
					interferanceGraphNode->GetInfo().m_name = variable;
					//dTrace (("%s\n", variable.GetStr()));
				}
			} else {
				Insert(variable);
			}
		}
	}

	for (dLiveInLiveOutSolver::dListNode* node = liveInLiveOut.GetFirst(); node; node = node->GetNext()) {
		dFlowGraphNode& point = node->GetInfo();
		dCILInstrCall* const functionCall = point.m_instruction->GetAsCall();
		if (functionCall) {
			const dCILInstr::dArg* const arg = functionCall->GetGeneratedVariable();
			dVariableSet<dString>::Iterator iter(point.m_livedOutputSet);
			for (iter.Begin(); iter; iter++) {
				const dString& var = iter.GetKey();
				if (var != arg->m_label) {
					dAssert (Find(var));
					dRegisterInterferenceNode& regNode = Find(var)->GetInfo();
					regNode.m_registerIndexBase = D_CALLEE_SAVE_REGISTER_START;
				}
			}
		}
	}


#ifdef D_TRACE_INTERFIRANCE_GRAPH
	dRegisterInterferenceGraph::Iterator debugIter(*this);
	for (debugIter.Begin(); debugIter; debugIter++) {
		const dString& key = debugIter.GetKey();
		dTrace(("%s\n", key.GetStr()));
	}
	dTrace(("\n"));
#endif

	// pre-color some special nodes
	int parameterInResgisterIndex = 0;
	for (dLiveInLiveOutSolver::dListNode* instNode = liveInLiveOut.GetFirst(); instNode; instNode = instNode->GetNext()) {
		dFlowGraphNode& point = instNode->GetInfo();
		dCILInstr* const instr = point.m_instruction;
		//instr->Trace();

		if (instr->GetAsArgument()) {
			dCILInstrArgument* const argInst = instr->GetAsArgument();
			const dCILInstr::dArg& arg = argInst->GetArg0();
			switch (arg.GetType().m_intrinsicType)
			{
				case dCILInstr::m_int:
				{
					if (parameterInResgisterIndex < D_PARMETER_IN_REGISTER_COUNT) {
						dTreeNode* const returnRegNode = Find(arg.m_label);
						dAssert(returnRegNode);
						dRegisterInterferenceNode& registerInfo = returnRegNode->GetInfo();
						dAssert(registerInfo.m_registerIndex == -1);
						registerInfo.m_registerIndex = D_PARMETER_IN_REGISTER_START + parameterInResgisterIndex;
						parameterInResgisterIndex++;
					} else {
						dAssert(0);
					}
					break;
				}

				case dCILInstr::m_luaType:
				{
					if (parameterInResgisterIndex < D_PARMETER_IN_REGISTER_COUNT) {
						dTreeNode* const returnRegNode = Find(arg.m_label);
						dAssert(returnRegNode);
						dRegisterInterferenceNode& registerInfo = returnRegNode->GetInfo();
						dAssert(registerInfo.m_registerIndex == -1);
						registerInfo.m_registerIndex = D_PARMETER_IN_REGISTER_START + parameterInResgisterIndex;
						parameterInResgisterIndex++;
					}
					else 
					{
						dAssert(0);
					}
					break;
				}

				default:
					dAssert(0);
			}

		} else if (instr->GetAsReturn()) {
			dCILInstrReturn* const retInstr = instr->GetAsReturn();

			const dCILInstr::dArg& arg = retInstr->GetArg0();
			//if (arg.GetType().m_isPointer || (arg.GetType().m_intrinsicType != dCILInstr::m_void)) {
			if (arg.GetType().m_intrinsicType != dCILInstr::m_void) {
				switch (arg.GetType().m_intrinsicType)
				{
					case dCILInstr::m_int:
					{
						dAssert (0);
						dTreeNode* const returnRegNode = Find(arg.m_label);
						dAssert(returnRegNode);
						dRegisterInterferenceNode& registerInfo = returnRegNode->GetInfo();
						dAssert(registerInfo.m_registerIndex == -1);
						registerInfo.m_registerIndex = D_RETURN_REGISTER_INDEX;
						break;
					}

					case dCILInstr::m_luaType:
					{
						dTreeNode* const returnRegNode = Find(arg.m_label);
						dAssert(returnRegNode);
						dRegisterInterferenceNode& registerInfo = returnRegNode->GetInfo();
						dAssert(registerInfo.m_registerIndex == -1);
						registerInfo.m_registerIndex = D_RETURN_REGISTER_INDEX;
						break;
					}

					case dCILInstr::m_constInt:
					case dCILInstr::m_constFloat:
					case dCILInstr::m_constString:
						break;		

					default:
					{
						dAssert(0);
					}
				}
			}

		} else if (instr->GetAsCall()) {
			dCILInstrCall* const callInstr = instr->GetAsCall();

			const dCILInstr::dArg& retArg = callInstr->GetArg0();
			//if (retArg.GetType().m_isPointer || (retArg.GetType().m_intrinsicType != dCILInstr::m_void)) {
			if (retArg.GetType().m_intrinsicType != dCILInstr::m_void) {
				switch (retArg.GetType().m_intrinsicType)
				{
					case dCILInstr::m_int:
					{
						dTreeNode* const returnRegNode = Find (retArg.m_label);
						dAssert(returnRegNode);
						dRegisterInterferenceNode& registerInfo = returnRegNode->GetInfo();
						dAssert(registerInfo.m_registerIndex == -1);
						registerInfo.m_registerIndex = D_RETURN_REGISTER_INDEX;
						break;
					}

					case dCILInstr::m_luaType:
					{
						dTreeNode* const returnRegNode = Find(retArg.m_label);
						dAssert(returnRegNode);
						dRegisterInterferenceNode& registerInfo = returnRegNode->GetInfo();
						dAssert(registerInfo.m_registerIndex == -1);
						registerInfo.m_registerIndex = D_RETURN_REGISTER_INDEX;
						break;
					}

					default:
						dAssert(0);
				}
			}

			int index = 0;
			for (dList<dCILInstr::dArg>::dListNode* node = callInstr->m_parameters.GetLast(); node && (index < D_PARMETER_IN_REGISTER_COUNT); node = node->GetPrev()) {
				const dCILInstr::dArg& arg = node->GetInfo();
				dTreeNode* const regNode = Find (arg.m_label);
				dAssert(regNode);
				dRegisterInterferenceNode& registerInfo = regNode->GetInfo();
				dAssert(registerInfo.m_registerIndex == -1);
				registerInfo.m_registerIndex = D_PARMETER_IN_REGISTER_START + index;
				index ++;
				dAssert (index < D_PARMETER_IN_REGISTER_COUNT);
			}
		}
	}

//m_graph->Trace();
	for (dLiveInLiveOutSolver::dListNode* node = liveInLiveOut.GetFirst(); node; node = node->GetNext()) {
		dFlowGraphNode& point = node->GetInfo();
//point.m_instruction->Trace();
		const dVariableSet<dString>& livedInputSet = point.m_livedInputSet;

		dVariableSet<dString>::Iterator iterA(livedInputSet);
		for (iterA.Begin(); iterA; iterA++) {
			const dString& varA = iterA.GetKey();
			dTree<dRegisterInterferenceNode, dString>::dTreeNode* const varAnodeNode = Find(varA);
			dRegisterInterferenceNode& varAnode = varAnodeNode->GetInfo();
			dVariableSet<dString>::Iterator iterB(iterA);
			for (iterB++; iterB; iterB++) {
				const dString& varB = iterB.GetKey();
				if (!varAnode.FindEdge(varB)) {
					dTree<dRegisterInterferenceNode, dString>::dTreeNode* const varBnodeNode = Find(varB);
					dRegisterInterferenceNode& varBnode = varBnodeNode->GetInfo();
					dRegisterInterferenceNodeEdge edgeAB(varBnodeNode);
					dRegisterInterferenceNodeEdge edgeBA(varAnodeNode);
					varAnode.m_interferanceEdge.Append(edgeAB);
					varBnode.m_interferanceEdge.Append(edgeBA);
#ifdef D_TRACE_INTERFIRANCE_GRAPH
					dTrace(("%s  %s\n", varA.GetStr(), varB.GetStr()));
#endif
					dAssert((varAnode.m_registerIndex == -1) || (varBnode.m_registerIndex == -1) || (varAnode.m_registerIndex != varBnode.m_registerIndex));
				}
			}
		}
	}

// remember implement register coalescing 
m_graph->Trace();

/*
	for (dLiveInLiveOutSolver::dListNode* node = liveInLiveOut.GetFirst(); node; node = node->GetNext()) {
		dFlowGraphNode& point = node->GetInfo();
		dCILInstr* const instr = point.m_instruction;
		if (instr->GetAsMove()) {
			dCILInstrMove* const move = instr->GetAsMove();
			dList<dCILInstr::dArg*> variablesList;
			move->GetUsedVariables(variablesList);
//move->Trace();
			if (variablesList.GetCount()) {
				//if ((stmt.m_instruction == dThreeAdressStmt::m_assigment) && (stmt.m_operator == dThreeAdressStmt::m_nothing) && (stmt.m_arg1.GetType().m_intrinsicType != dThreeAdressStmt::m_constInt) && (stmt.m_arg1.GetType().m_intrinsicType != dThreeAdressStmt::m_constFloat)) {
				const dString& variableA = move->GetArg0().m_label;
				const dString& variableB = move->GetArg1().m_label;
				dTreeNode* const nodeA = Find(variableA);
				dTreeNode* const nodeB = Find(variableB);
				nodeA;
				nodeB;
			}
		}
	}
*/
}


void dRegisterInterferenceGraph::ApplyDeadCodeElimination()
{
	for (dCIL::dListNode* node = m_graph->m_begin; node != m_graph->m_end; node = node->GetNext()) {
		dCILInstrMove* const moveInst = node->GetInfo()->GetAsMove();
		if (moveInst) {
		//moveInst->Trace();
			dList<dCILInstr::dArg*> variablesList;
			dCILInstr::dArg* const genVariable = moveInst->GetGeneratedVariable();
			moveInst->GetUsedVariables(variablesList);
			if (variablesList.GetCount() && (genVariable->m_label == variablesList.GetFirst()->GetInfo()->m_label)) {
				moveInst->Nullify();
			}
		}
	}

	for (bool anyChanges = true; anyChanges; ) {
		anyChanges = false;
		dLiveInLiveOutSolver liveInLiveOut(m_graph);
		for (dLiveInLiveOutSolver::dListNode* node = liveInLiveOut.GetFirst(); node; node = node->GetNext() ) {
			const dFlowGraphNode& liveInfo = node->GetInfo();
			const dCILInstr::dArg* const variableOuter = liveInfo.m_instruction->GetGeneratedVariable();
		//liveInfo.m_instruction->Trace();
			if (variableOuter && !liveInfo.m_instruction->GetAsCall()) {
				if (!liveInfo.m_livedOutputSet.Find(variableOuter->m_label)) {
					anyChanges = true;
					dAssert (0);
					//liveInfo.m_instruction->Trace();
					liveInfo.m_instruction->Nullify();
				}
			}
		}
	}
}


bool dRegisterInterferenceGraph::RemoveNop()
{
	bool ret = false;
	dCIL::dListNode* nextNode;
	for (dCIL::dListNode* node = m_graph->m_begin; node && !node->GetInfo()->GetAsFunctionEnd(); node = nextNode) {
		nextNode = node->GetNext();
		if (node->GetInfo()->GetAsNop()) {
			delete node->GetInfo();
			ret = true;
		}
	}
	return ret;
}


bool dRegisterInterferenceGraph::RemoveRedundantJumps ()
{
	bool ret = false;

	RemoveNop();

	// create jump and label workList;
	dTree<int, dCIL::dListNode*> jumpWorkList;
	for (dCIL::dListNode* node = m_graph->m_begin->GetNext(); node && !node->GetInfo()->GetAsFunctionEnd(); node = node->GetNext()) {
		dCILInstr* const instr = node->GetInfo();
		if (instr->GetAsIF() || instr->GetAsLabel() || instr->GetAsGoto()) {
			jumpWorkList.Insert(0, node);
		}
	}

//m_graph->m_begin->GetInfo()->GetCil()->Trace();

	dTree<int, dCIL::dListNode*>::Iterator iter(jumpWorkList);

	// remove redundant adjacent labels
	for (iter.Begin(); iter; iter++) {
		dCIL::dListNode* const labelNode = iter.GetKey();
		const dCILInstrLabel* const stmt = labelNode->GetInfo()->GetAsLabel();
		if (stmt) {
//stmt->Trace();
			dCIL::dListNode* nextNode = labelNode->GetNext();
			while (nextNode && nextNode->GetInfo()->GetAsNop()) {
				nextNode = nextNode->GetNext();
			}

			if (nextNode->GetInfo()->GetAsLabel()) {
//nextNode->GetInfo()->Trace();
				dTree<int, dCIL::dListNode*>::Iterator iter1(jumpWorkList);
				for (iter1.Begin(); iter1; iter1++) {
					dCIL::dListNode* const node1 = iter1.GetKey();
					dCILInstrGoto* const gotoInstru = node1->GetInfo()->GetAsGoto();
					if (gotoInstru) {
						if (gotoInstru->GetTarget() == nextNode) {
							ret = true;
							gotoInstru->Trace();
							gotoInstru->SetTarget(labelNode->GetInfo()->GetAsLabel());
						}
					}
					dCILInstrConditional* const ifInstru = node1->GetInfo()->GetAsIF();
					if (ifInstru) {
						if (ifInstru->GetTrueTarget() == nextNode) {
							ret = true;
							//ifInstru->Trace();
							ifInstru->SetTargets(labelNode->GetInfo()->GetAsLabel(), ifInstru->GetFalseTarget()->GetInfo()->GetAsLabel());
							//ifInstru->Trace();
						}
					}
				}
			}
		}
	}

	// redirect double indirect jumps
	for (iter.Begin(); iter; iter ++) {
		dCIL::dListNode* const node = iter.GetKey();
	
		if (node->GetInfo()->GetAsGoto()) {
			dCILInstrGoto* const stmt = node->GetInfo()->GetAsGoto();
//stmt->Trace();

			dAssert (jumpWorkList.Find (stmt->GetTarget()));
			dCIL::dListNode* const targetNode = jumpWorkList.Find (stmt->GetTarget())->GetKey();
			dCILInstr* stmt1 = targetNode->GetInfo();
//stmt1->Trace();
			dCIL::dListNode* nextNode = targetNode->GetNext();
			while (nextNode && nextNode->GetInfo()->GetAsNop()) {
				nextNode = nextNode->GetNext();
			}

			if (stmt1->GetAsLabel() && nextNode->GetInfo()->GetAsGoto()) {
				//stmt->Trace();
				//nextNode->GetInfo()->Trace();
				const dCILInstrGoto* const stmt2 = nextNode->GetInfo()->GetAsGoto();
				stmt->SetTarget(stmt2->GetTarget()->GetInfo()->GetAsLabel());
				//stmt->Trace();
				ret = true;

			}
		} else if (node->GetInfo()->GetAsIF()) {
			dCILInstrConditional* const stmt = node->GetInfo()->GetAsIF();
//	stmt->Trace();
			dAssert (jumpWorkList.Find (stmt->GetTrueTarget()));
			dCIL::dListNode* const targetNode = jumpWorkList.Find (stmt->GetTrueTarget())->GetKey();
			dCILInstr* stmt1 = targetNode->GetInfo();
//stmt1->Trace();
			dCIL::dListNode* nextNode = targetNode->GetNext();
			while (nextNode && nextNode->GetInfo()->GetAsNop()) {
				nextNode = nextNode->GetNext();
			}

			if (stmt1->GetAsLabel() && nextNode->GetInfo()->GetAsGoto()) {
				dCILInstrGoto* const gotoInstruct = nextNode->GetInfo()->GetAsGoto();
				stmt->SetTargets (gotoInstruct->GetTarget()->GetInfo()->GetAsLabel(), stmt->GetFalseTarget()->GetInfo()->GetAsLabel());
//stmt->Trace();
				ret = true;
			}
		}
	}
//m_graph->m_begin->GetInfo()->GetCil()->Trace();

	// remove goto to immediate labels
	for (iter.Begin(); iter; iter ++) {
		dCIL::dListNode* const node = iter.GetKey();
		dCILInstrGoto* const stmt = node->GetInfo()->GetAsGoto();
		if (stmt) {
//stmt->Trace();
			dCIL::dListNode* nextNode = node->GetNext();
			while (nextNode && nextNode->GetInfo()->GetAsNop()) {
				nextNode = nextNode->GetNext();
			}

			if (nextNode->GetInfo()->GetAsLabel()) {
				if (stmt->GetLabel() == nextNode->GetInfo()->GetAsLabel()->GetLabel()) {
//nextNode->GetInfo()->Trace();
					ret = true;
					stmt->Nullify();
				}
			}
		}
	}

//m_graph->m_begin->GetInfo()->GetCil()->Trace();

	// delete unreachable labels
	for (iter.Begin(); iter; ) {
		dCIL::dListNode* const labelNode = iter.GetKey();
		const dCILInstrLabel* const stmt = labelNode->GetInfo()->GetAsLabel();
		iter++;

		if (stmt) {
			bool isReferenced = false;
			dTree<int, dCIL::dListNode*>::Iterator iter1 (jumpWorkList);
			for (iter1.Begin(); iter1; iter1 ++) {
				dCIL::dListNode* const node1 = iter1.GetKey();
				dCILInstrGoto* const gotoInstru = node1->GetInfo()->GetAsGoto();
				if (gotoInstru){
					if (gotoInstru->GetTarget() == labelNode) {
						isReferenced = true;
						break;
					}
				} 
				dCILInstrConditional* const ifInstru = node1->GetInfo()->GetAsIF();
				if (ifInstru) {
					if ((ifInstru->GetTrueTarget() == labelNode) || (ifInstru->GetFalseTarget() == labelNode)) {
						isReferenced = true;
						break;
					}
				}
			}
			if (!isReferenced) {
				//stmt->Trace();
				//m_graph->m_begin->GetInfo()->GetCil()->Trace();
				labelNode->GetInfo()->Nullify();
				//m_graph->m_begin->GetInfo()->GetCil()->Trace();
			}
		}
	}

//m_graph->m_begin->GetInfo()->GetCil()->Trace();
	return ret;
}

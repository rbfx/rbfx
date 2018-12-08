/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneGraph.h
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
#include "dNodeInfo.h"
#include "dSceneNodeInfo.h"
#include "dSceneGraph.h"
#include <tinyxml.h>



dGraphNode::dGraphNode ()
	:m_lru (0),  m_nodeInfo(NULL), m_parents(), m_children()
{
}

dGraphNode::dGraphNode (const dGraphNode& node)
	:m_lru (0),  m_nodeInfo(NULL), m_parents(), m_children()
{
	dAssert (0);
}

dGraphNode::~dGraphNode ()
{
}

void dGraphNode::SetNode(dNodeInfo* const newInfo)
{
	m_nodeInfo->Release();
	m_nodeInfo = newInfo;
	m_nodeInfo->AddRef();
}

dSceneGraph::dSceneGraph(dNodeInfo* const rootInfo)
	:dTree<dGraphNode, unsigned>(), m_lru (0)
{
	m_rootNode = AddNode (rootInfo, NULL);
	rootInfo->Release();
}

dSceneGraph::dSceneGraph(const dSceneGraph& me)
	:dTree<dGraphNode, unsigned>(), m_lru (0)
{
	// add all nodes from me,  
	Iterator iter (me);
	for (iter.Begin(); iter; iter ++) {
		dGraphNode& srcNode = iter.GetNode()->GetInfo();
		AddNode (srcNode.m_nodeInfo, NULL);
	}

	//now connect all edges
	for (iter.Begin(); iter; iter ++) {
		dGraphNode& srcNode = iter.GetNode()->GetInfo();
		dGraphNode& myNode = Find(srcNode.m_nodeInfo->GetUniqueID())->GetInfo();

		for (dGraphNode::dLink::dListNode* srcEdge = srcNode.m_children.GetFirst(); srcEdge; srcEdge = srcEdge->GetNext()) {
			dGraphNode& srcLinkNode = srcEdge->GetInfo()->GetInfo();
			dTreeNode* myLinkNode = Find(srcLinkNode.m_nodeInfo->GetUniqueID());
			//myNode.m_children.Append(srcEdge->GetInfo());
			myNode.m_children.Append(myLinkNode);
		}

		for (dGraphNode::dLink::dListNode* srcEdge = srcNode.m_parents.GetFirst(); srcEdge; srcEdge = srcEdge->GetNext()) {
			dGraphNode& srcLinkNode = srcEdge->GetInfo()->GetInfo();
			dTreeNode* myLinkNode = Find(srcLinkNode.m_nodeInfo->GetUniqueID());
			//myNode.m_parents.Append(srcEdge->GetInfo());
			myNode.m_parents.Append(myLinkNode);
		}
	}

	m_rootNode = Find(me.GetRootNode()->GetInfo().GetNode()->GetUniqueID());
}

dSceneGraph::~dSceneGraph(void)
{
	Cleanup();
}

void dSceneGraph::Cleanup()
{
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dGraphNode& info = (*iter);
		info.m_parents.RemoveAll();
		info.m_children.RemoveAll();
		info.m_nodeInfo->Release();
		info.m_nodeInfo = NULL;
	}
	RemoveAll();
	m_rootNode = NULL;
}

int dSceneGraph::GetLRU()
{
	m_lru ++;
	return m_lru;
}


bool dSceneGraph::HasLinkToRoot (dTreeNode* const nodeIn)
{
	int stack;
	char parentEdge[D_GRAPH_MAX_STACK_DEPTH];
	dTreeNode* pool[D_GRAPH_MAX_STACK_DEPTH];

	pool[0] = nodeIn;
	stack = 1;

	int lru = GetLRU();
	nodeIn->GetInfo().SetLRU (lru);

	while (stack) {
		stack --;
		dTreeNode* const node = pool[stack];
		if (node == m_rootNode) {
			return true;
		}

		int index = 0;
		for (index = stack - 1; (index >= 0) && parentEdge[index]; index --);
		if ((index >= 0) && !parentEdge[index]) {
			index ++;
			dAssert (index <= stack);
		}
		
		for (dGraphNode::dLink::dListNode* link = node->GetInfo().m_children.GetFirst(); link; link = link->GetNext()){
			dGraphNode& info = link->GetInfo()->GetInfo();
			if (info.GetLRU() != lru) {

				info.SetLRU(lru);
				if (index >= 0) {
					pool[stack] = pool[index];
					parentEdge[stack] = parentEdge[index];

					pool[index] = link->GetInfo();
					parentEdge[index] = 0;
					index ++;
				} else {
					pool[stack] = link->GetInfo();
					parentEdge[stack] = 0;
				}
				stack ++;
				dAssert (stack < int (sizeof (parentEdge) / sizeof (parentEdge[0])));
			}
		}

		for (dGraphNode::dLink::dListNode* link = node->GetInfo().m_parents.GetFirst(); link; link = link->GetNext()){
			dGraphNode& info = link->GetInfo()->GetInfo();
			if (info.GetLRU() != lru) {
				info.SetLRU(lru);
				pool[stack] = link->GetInfo();
				parentEdge[stack] = 1;
				stack ++;
				dAssert (stack < int (sizeof (parentEdge) / sizeof (parentEdge[0])));
			}
		}
	}
	return false;
}

void dSceneGraph::SetNodeInfo (dNodeInfo* const newInfo, dTreeNode* const node)
{
	newInfo->AddRef();
	dGraphNode& graphNode = node->GetInfo();
	dNodeInfo* const oldInfo = graphNode.m_nodeInfo;
	newInfo->SetUniqueID(oldInfo->GetUniqueID());
	oldInfo->Release();
	graphNode.m_nodeInfo = newInfo;
}


void dSceneGraph::AddEdge (dTreeNode* const parent, dTreeNode* const child)
{
	dGraphNode& parentNode = parent->GetInfo();
	dGraphNode& childNode = child->GetInfo();

	dGraphNode::dLink::dListNode* parentLink = NULL;
	dGraphNode::dLink::dListNode* childLink = NULL;

	for (childLink = childNode.m_parents.GetFirst(); childLink; childLink = childLink->GetNext()) {
		if (childLink->GetInfo() == parent) {
			for (parentLink = parentNode.m_children.GetFirst(); parentLink; parentLink = parentLink->GetNext()){
				if (parentLink->GetInfo() == child) {
					break;
				}
			}
			break;
		}
	}

	dAssert ((parentLink && childLink) || (!parentLink && !childLink));
	if (!parentLink && !childLink) {
		parentNode.m_children.Append(child);
		childNode.m_parents.Append(parent);
	}
}

void dSceneGraph::UnlinkEdge (dTreeNode* const node1, dTreeNode* const node2)
{
dAssert (0);
/*
	dGraphNode& info1 = node1->GetInfo();
	dGraphNode& info2 = node2->GetInfo();

	dGraphNode::dListNode* link1 = NULL;
	dGraphNode::dListNode* link2 = NULL;
	for (link1 = info1.GetFirst(); link1; link1 = link1->GetNext()){
		if (link1->GetInfo().GetNode() == node2) {
			for (link2 = info2.GetFirst(); link2; link2 = link2->GetNext()) {
				if (link2->GetInfo().GetNode() == node1) {
					break;
				}
			}
			break;
		}
	}

	dAssert ((link1 && link2) || (!link1 && !link2));
	if (link1 && link2) {
		info1.Remove(link1);
		info2.Remove(link2);
	}
*/
}

void dSceneGraph::DeleteEdge (dTreeNode* const node1, dTreeNode* const node2)
{
	dGraphNode& info1 = node1->GetInfo();
	dGraphNode& info2 = node2->GetInfo();

	for (dGraphNode::dLink::dListNode* link1 = info1.m_children.GetFirst(); link1; link1 = link1->GetNext()){
		if (link1->GetInfo() == node2) {
			for (dGraphNode::dLink::dListNode* link2 = info2.m_parents.GetFirst(); link2; link2 = link2->GetNext()) {
				if (link2->GetInfo() == node1) {
					info1.m_children.Remove(link1);
					info2.m_parents.Remove(link2);

					if (!HasLinkToRoot (node1)) {
						dAssert (node1 != m_rootNode);
						DeleteNode (node1);
					}
					if (!HasLinkToRoot (node2)) {
						dAssert (node2 != m_rootNode);
						DeleteNode (node2);
					}
					return;
				}
			}
		}
	}

	for (dGraphNode::dLink::dListNode* link1 = info1.m_parents.GetFirst(); link1; link1 = link1->GetNext()){
		if (link1->GetInfo() == node2) {
			for (dGraphNode::dLink::dListNode* link2 = info2.m_children.GetFirst(); link2; link2 = link2->GetNext()) {
				if (link2->GetInfo() == node1) {
					info1.m_parents.Remove(link1);
					info2.m_children.Remove(link2);

					if (!HasLinkToRoot (node1)) {
						dAssert (node1 != m_rootNode);
						DeleteNode (node1);
					}
					if (!HasLinkToRoot (node2)) {
						dAssert (node2 != m_rootNode);
						DeleteNode (node2);
					}
					return;
				}
			}
		}
	}
}


dSceneGraph::dTreeNode* dSceneGraph::AddNode (dNodeInfo* const info, dTreeNode* const parent)
{
	dTreeNode* const child = Insert(info->GetUniqueID());
	dGraphNode& node = child->GetInfo();
	node.m_nodeInfo = info;
	info->AddRef();

	if (parent) {
		parent->GetInfo().m_children.Append(child);
		child->GetInfo().m_parents.Append(parent);
	}
	return child;
}


void dSceneGraph::DeleteNode (dTreeNode* const nodeIn)
{
	dList<dTreeNode*> conectedNodes;
	dTree<dTreeNode*, unsigned> deleteList;
	dGraphNode& infoIn = nodeIn->GetInfo();
	deleteList.Insert(nodeIn, nodeIn->GetKey());

	while (infoIn.m_parents.GetFirst()) {
		dTreeNode* const twinNode = infoIn.m_parents.GetFirst()->GetInfo();

		conectedNodes.Append (twinNode);
		dGraphNode& twinInfo = twinNode->GetInfo();
		for (dGraphNode::dLink::dListNode* link = twinInfo.m_children.GetFirst(); link; link = link->GetNext()){
			if (link->GetInfo() == nodeIn) {
				twinInfo.m_children.Remove (link);
				break;
			}
		}
		for (dGraphNode::dLink::dListNode* link = twinInfo.m_parents.GetFirst(); link; link = link->GetNext()){
			if (link->GetInfo() == nodeIn) {
				twinInfo.m_parents.Remove (link);
				break;
			}
		}
		infoIn.m_parents.Remove(infoIn.m_parents.GetFirst());
	}

	
	while (infoIn.m_children.GetFirst()) {
		dTreeNode* const twinNode = infoIn.m_children.GetFirst()->GetInfo();

		conectedNodes.Append (twinNode);
		dGraphNode& twinInfo = twinNode->GetInfo();
		for (dGraphNode::dLink::dListNode* link = twinInfo.m_children.GetFirst(); link; link = link->GetNext()){
			if (link->GetInfo() == nodeIn) {
				twinInfo.m_children.Remove (link);
				break;
			}
		}
		for (dGraphNode::dLink::dListNode* link = twinInfo.m_parents.GetFirst(); link; link = link->GetNext()){
			if (link->GetInfo() == nodeIn) {
				twinInfo.m_parents.Remove (link);
				break;
			}
		}
		infoIn.m_children.Remove(infoIn.m_children.GetFirst());
	}

	
	for (dList<dTreeNode*>::dListNode* ptr = conectedNodes.GetFirst(); ptr; ptr = ptr->GetNext()){
		dTreeNode* pool[D_GRAPH_MAX_STACK_DEPTH];
		
		int stack = 1;
		int lru = GetLRU();
		pool[0] = ptr->GetInfo();
		nodeIn->GetInfo().SetLRU (lru);
		bool listIsDangling = true;
		dList<dTreeNode*> danglingNodes;

		while (stack) {
			stack --;
			dTreeNode* const node = pool[stack];
			if (node == m_rootNode) {
				listIsDangling = false;
				break;
			}
			danglingNodes.Append(node);

			for (dGraphNode::dLink::dListNode* link = node->GetInfo().m_children.GetFirst(); link; link = link->GetNext()){
				dGraphNode& info = link->GetInfo()->GetInfo();

				if (info.GetLRU() != lru) {
					info.SetLRU(lru);
					pool[stack] = link->GetInfo();
					stack ++;
					dAssert (stack < int (sizeof (pool) / sizeof (pool[0])));
				}
			}

			for (dGraphNode::dLink::dListNode* link = node->GetInfo().m_parents.GetFirst(); link; link = link->GetNext()){
				dGraphNode& info = link->GetInfo()->GetInfo();

				if (info.GetLRU() != lru) {
					info.SetLRU(lru);
					pool[stack] = link->GetInfo();
					stack ++;
					dAssert (stack < int (sizeof (pool) / sizeof (pool[0])));
				}
			}
		}

		if (listIsDangling) {
			for (dList<dTreeNode*>::dListNode* first = danglingNodes.GetFirst(); first; first = first->GetNext()) {
				deleteList.Insert(first->GetInfo(), first->GetInfo()->GetKey());
			}
		}
		danglingNodes.RemoveAll();
	}

	dTree<dTreeNode*, unsigned>::Iterator iter (deleteList);
	for (iter.Begin(); iter; iter ++) {
		dTreeNode* node = (*iter);
		if (Find(node->GetKey())) {
			dGraphNode& info = node->GetInfo();
			info.m_parents.RemoveAll();
			info.m_children.RemoveAll();
			info.m_nodeInfo->Release();
			info.m_nodeInfo = NULL;
			Remove (node);
		}
	}
}


dSceneGraph::dTreeNode* dSceneGraph::GetRootNode() const
{
	dAssert (m_rootNode);
	return m_rootNode;
}


void dSceneGraph::AddRootNode(dNodeInfo* const info)
{
	dAssert (!m_rootNode);
	m_rootNode = AddNode(info, NULL);
}


void dSceneGraph::DeleteRootNode()
{
	dAssert (m_rootNode);
	DeleteNode(m_rootNode);
	m_rootNode = NULL;
}


void dSceneGraph::Serialize (TiXmlElement* const rootNode) const
{
	// save scenes nodes
	TiXmlElement* const nodes = new TiXmlElement ("nodes");
	rootNode->LinkEndChild(nodes);
	
	dTree<int, dTreeNode*> enumerator;

	int index = 0;
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dTreeNode* node = iter.GetNode();
		//dNodeInfo* info = node->GetInfo().GetNode();
		enumerator.Insert (index, node);
		index ++;
	}
	nodes->SetAttribute("count", index);

	int indexList[D_GRAPH_MAX_STACK_DEPTH];
	char text[D_GRAPH_MAX_STACK_DEPTH * 32];

	for (iter.Begin(); iter; iter ++) {
		dTreeNode* const node = iter.GetNode();
		dNodeInfo* info = node->GetInfo().GetNode();
		TiXmlElement* const infoNode = new TiXmlElement (info->GetClassName());
		nodes->LinkEndChild(infoNode);
		info->Serialize (infoNode);
		dAssert (infoNode);

		int nodeCount = 0;
		for (dGraphNode::dLink::dListNode* edgeNode = node->GetInfo().m_parents.GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
			dTree<int, dTreeNode*>::dTreeNode* const edge = enumerator.Find (edgeNode->GetInfo());
			dAssert (edge);
//			if (edge) {
				indexList[nodeCount] = edge->GetInfo();
				nodeCount ++;
				dAssert (nodeCount < int (sizeof (indexList) / sizeof (indexList[0])));
//			}
		}
		if (nodeCount) {
			dIntArrayToString (indexList, nodeCount, text, sizeof (text));
			TiXmlElement* const parent = new TiXmlElement ("parentNodes");
			parent->SetAttribute("count", nodeCount);
			parent->SetAttribute("indices", text);
			infoNode->LinkEndChild(parent);
		}
		
		nodeCount = 0;
		for (dGraphNode::dLink::dListNode* edgeNode = node->GetInfo().m_children.GetFirst(); edgeNode; edgeNode = edgeNode->GetNext()) {
			dTree<int, dTreeNode*>::dTreeNode* const edge = enumerator.Find (edgeNode->GetInfo());
			dAssert (edge);
//			if (edge) {
				indexList[nodeCount] = edge->GetInfo();
				nodeCount ++;
				dAssert (nodeCount < int (sizeof (indexList) / sizeof (indexList[0])));
//			}
		}
		if (nodeCount) {
			dIntArrayToString (indexList, nodeCount, text, sizeof (text));
			TiXmlElement* const parent = new TiXmlElement ("childrenNodes");
			parent->SetAttribute("count", nodeCount);
			parent->SetAttribute("indices", text);
			infoNode->LinkEndChild(parent);
		}
	}
}

bool dSceneGraph::Deserialize (TiXmlElement* const rootNode) 
{
	Cleanup();

	dScene* const world = (dScene*) this;
	for (TiXmlElement* element = (TiXmlElement*) rootNode->FirstChild(); element; element = (TiXmlElement*) element->NextSibling()) {
		const char* const className = element->Value();
		dNodeInfo* const info = dNodeInfo::CreateFromClassName (className, world);
		if (info) {
			info->Deserialize((dScene*)this, element);
			AddNode (info, NULL);
			info->Release();
		}
	}

	int baseIndex = Minimum()->GetKey();
	int baseIndexCount = baseIndex;
	for (TiXmlElement* element = (TiXmlElement*) rootNode->FirstChild(); element; element = (TiXmlElement*) element->NextSibling()) {
		dTreeNode* const myNode = Find (baseIndexCount);
		baseIndexCount ++;
		if (myNode) {
			dAssert (myNode);
			dGraphNode& node = myNode->GetInfo();

			TiXmlElement* const parentNodes = (TiXmlElement*) element->FirstChild ("parentNodes");
			if (parentNodes) {
				int count;
				parentNodes->Attribute("count", &count);
				const char* const indices = parentNodes->Attribute ("indices");

				const char* ptr = indices;
				for (int i = 0; i < count; i ++) {
					char index[128];
					sscanf (ptr, "%s", index);
					ptr = strstr (ptr, index);
					ptr += strlen (index); 
					int parentIndex = atoi (index);

					dTreeNode* parentNode = Find(parentIndex + baseIndex);
					dAssert (parentNode);
					node.m_parents.Append(parentNode);
				}
			} else {
				if (!m_rootNode) {
					m_rootNode = myNode;
				}
			}


			TiXmlElement* const childrenNodes = (TiXmlElement*) element->FirstChild ("childrenNodes");
			if (childrenNodes) {
				int count;
				childrenNodes->Attribute("count", &count);
				const char* const indices = childrenNodes->Attribute ("indices");

				const char* ptr = indices;
				for (int i = 0; i < count; i ++) {
					char index[128];
					sscanf (ptr, "%s", index);
					ptr = strstr (ptr, index);
					ptr += strlen (index); 
					int childIndex = atoi (index);

					dTreeNode* const childNode = Find(childIndex + baseIndex);
					if (childNode) {
						dAssert (childNode);
						node.m_children.Append(childNode);
					}
				}
			}
		}
	}

	return true;
}




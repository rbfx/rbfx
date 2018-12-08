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

#include "dContainersStdAfx.h"
#include "dTree.h"


dRedBackNode* dRedBackNode::Minimum () const
{
	dRedBackNode* ptr = (dRedBackNode*) this;
	for (; ptr->m_left; ptr = ptr->m_left);
	return ptr;
}

dRedBackNode* dRedBackNode::Maximum () const
{
	dRedBackNode* ptr = (dRedBackNode*) this;
	for (; ptr->m_right; ptr = ptr->m_right);
	return ptr;
}


dRedBackNode* dRedBackNode::Prev () const
{
	if (m_left) {
		return m_left->Maximum ();
	}

	dRedBackNode* me = (dRedBackNode*) this;
	dRedBackNode* ptr = m_parent;
	for (; ptr && me == ptr->m_left; ptr = ptr->m_parent) {
		me = ptr;
	}
	return ptr;

}

dRedBackNode* dRedBackNode::Next () const
{
	if (m_right) {
		return m_right->Minimum ();
	}

	dRedBackNode* node = (dRedBackNode*) this;
	dRedBackNode* ptr = m_parent;
	for (; ptr && node == ptr->m_right; ptr = ptr->m_parent) {
		node = ptr;
	}
	return ptr;
}

// rotate node me to left 
void dRedBackNode::RotateLeft(dRedBackNode** const head) 
{
	dRedBackNode* const me = this;
	dRedBackNode* const child = me->m_right;
	
	dAssert (child);
	//establish me->m_right link 
	me->m_right = child->m_left;
	if (child->m_left != NULL) {
		child->m_left->m_parent = me;
	}
	
	// establish child->m_parent link 
	if (child != NULL) {
		child->m_parent = me->m_parent;
	}
	if (me->m_parent) {
		if (me == me->m_parent->m_left) {
			me->m_parent->m_left = child;
		} else {
			me->m_parent->m_right = child;
		}
	} else {
		*head = child;
	}
	
	// link child and me 
	child->m_left = me;
	if (me != NULL) {
		me->m_parent = child;
	}
}


// rotate node me to right  *
void dRedBackNode::RotateRight(dRedBackNode** const head) 
{
	dRedBackNode* const me = this;
	dRedBackNode* const child = me->m_left;

	dAssert (child);
	// establish me->m_left link 
	me->m_left = child->m_right;
	if (child->m_right != NULL) {
		child->m_right->m_parent = me;
	}

	// establish child->m_parent link 
	if (child != NULL) {
		child->m_parent = me->m_parent;
	}
	if (me->m_parent) {
		if (me == me->m_parent->m_right) {
			me->m_parent->m_right = child;
		} else {
			me->m_parent->m_left = child;
		}
	} else {
		*head = child;
	}

	// link me and child 
	child->m_right = me;
	if (me != NULL) {
		me->m_parent = child;
	}
}


// maintain Red-Black tree balance after inserting node ptr   
void dRedBackNode::InsertFixup(dRedBackNode** const head) 
{
	dRedBackNode* ptr = this;
	// check Red-Black properties 
	dAssert ((ptr == *head) || ptr->m_parent);
	while ((ptr != *head) && (ptr->m_parent->GetColor() == RED)) {
		// we have a violation 
		dAssert (ptr->m_parent);
		dAssert (ptr->m_parent->m_parent);
		if (ptr->m_parent == ptr->m_parent->m_parent->m_left) {
			dRedBackNode* const tmp = ptr->m_parent->m_parent->m_right;
			if (tmp && (tmp->GetColor() == RED)) {
				// uncle is RED 
				ptr->m_parent->SetColor(BLACK);
				tmp->SetColor(BLACK) ;
				ptr->m_parent->m_parent->SetColor(RED) ;
				ptr = ptr->m_parent->m_parent;
			} else {
				// uncle is BLACK 
				if (ptr == ptr->m_parent->m_right) {
					// make ptr a left child 
					ptr = ptr->m_parent;
					ptr->RotateLeft(head);
				}

				ptr->m_parent->SetColor(BLACK);
				if (ptr->m_parent->m_parent) {
					ptr->m_parent->m_parent->SetColor(RED);
					ptr->m_parent->m_parent->RotateRight(head);
				}
			}
		} else {
			//dAssert (ptr->m_parent == ptr->m_parent->m_parent->m_right);
			// mirror image of above code 
			dRedBackNode* const tmp = ptr->m_parent->m_parent->m_left;
			if (tmp && (tmp->GetColor() == RED)) {
				//uncle is RED 
				ptr->m_parent->SetColor(BLACK);
				tmp->SetColor(BLACK) ;
				ptr->m_parent->m_parent->SetColor(RED) ;
				ptr = ptr->m_parent->m_parent;
			} else {
				// uncle is BLACK 
				if (ptr == ptr->m_parent->m_left) {
					ptr = ptr->m_parent;
					ptr->RotateRight(head);
				}
				ptr->m_parent->SetColor(BLACK);
				if (ptr->m_parent->m_parent->GetColor() == BLACK) {
					ptr->m_parent->m_parent->SetColor(RED) ;
				   ptr->m_parent->m_parent->RotateLeft (head); 
				}
			}
		}
	}
	(*head)->SetColor(BLACK);
}


//maintain Red-Black tree balance after deleting node x 
void dRedBackNode::RemoveFixup (dRedBackNode* const me, dRedBackNode** const head) 
{
	dRedBackNode* ptr = this;
	dRedBackNode* node = me;
	while ((node != *head) && (!node || node->GetColor() == BLACK)) {
		if (node == ptr->m_left) {
			if (!ptr) {
				return;
			}
			dRedBackNode* tmp = ptr->m_right;
			if (!tmp) {
				return;
			}
			if (tmp->GetColor() == RED) {
				tmp->SetColor(BLACK) ;
				ptr->SetColor(RED) ;
				ptr->RotateLeft (head);
				tmp = ptr->m_right;
				//if (!ptr || !tmp) {
				if (!tmp) {
					return;
				}
			}
			if ((!tmp->m_left  || (tmp->m_left->GetColor() == BLACK)) && 
				 (!tmp->m_right || (tmp->m_right->GetColor() == BLACK))) {
				tmp->SetColor(RED);
				node = ptr;
				ptr = ptr->m_parent;
				continue;
			} else if (!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) {
				tmp->m_left->SetColor(BLACK);
				tmp->SetColor(RED);
				tmp->RotateRight (head);
				tmp = ptr->m_right;
				//if (!ptr || !tmp) {
				if (!tmp) {
					return;
				}
			}
			tmp->SetColor (ptr->GetColor());
			if (tmp->m_right) {
				tmp->m_right->SetColor(BLACK) ;
			}
			if (ptr) {
				ptr->SetColor(BLACK) ;
				ptr->RotateLeft (head);
			}
			node = *head;

		} else {
		  	if (!ptr) {
				return;
		  	}
			dRedBackNode* tmp = ptr->m_left;
			if (!tmp) {
				return;
			}
			if (tmp->GetColor() == RED) {
				tmp->SetColor(BLACK) ;
				ptr->SetColor(RED) ;
				ptr->RotateRight (head);
				tmp = ptr->m_left;
				//if (!ptr || !tmp) {
				if (!tmp) {
					return;
				}
			}

			if ((!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) && 
				 (!tmp->m_left  || (tmp->m_left->GetColor() == BLACK))) {
				tmp->SetColor(RED) ;
				node = ptr;
				ptr = ptr->m_parent;
				continue;
			} else if (!tmp->m_left || (tmp->m_left->GetColor() == BLACK)) {
				tmp->m_right->SetColor(BLACK) ;
				tmp->SetColor(RED) ;
				tmp->RotateLeft (head);
				tmp = ptr->m_left;
				//if (!ptr || !tmp) {
				if (!tmp) {
					return;
				}
			}
			tmp->SetColor (ptr->GetColor());
			if (tmp->m_left) {
				tmp->m_left->SetColor(BLACK);
			}
			if (ptr) {
				ptr->SetColor(BLACK) ;
				ptr->RotateRight (head);
			}
			node = *head;
		}
	}
	if (node) {
		node->SetColor(BLACK);
	}
}

void dRedBackNode::Unlink (dRedBackNode** const head)
{
	dRedBackNode* const node = this;
	node->SetInTreeFlag(false);

	if (!node->m_left || !node->m_right) {
		// y has a NULL node as a child 
		dRedBackNode* const endNode = node;

		// x is y's only child 
		dRedBackNode* child = endNode->m_right;
		if (endNode->m_left) {
			child = endNode->m_left;
		}

		// remove y from the parent chain 
		if (child) {
			child->m_parent = endNode->m_parent;
		}

		if (endNode->m_parent)	{
			if (endNode == endNode->m_parent->m_left) {
				endNode->m_parent->m_left = child;
			} else {
				endNode->m_parent->m_right = child;
			}
		} else {
			*head = child;
		}

		if (endNode->GetColor() == BLACK) {
			endNode->m_parent->RemoveFixup (child, head);
		}
	} else {

		// find tree successor with a NULL node as a child 
		dRedBackNode* endNode = node->m_right;
		while (endNode->m_left != NULL) {
			endNode = endNode->m_left;
		}

		// x is y's only child 
		dRedBackNode* const child = endNode->m_right;

		endNode->m_left = node->m_left;
		node->m_left->m_parent = endNode;

		dRedBackNode* endNodeParent = endNode;
		if (endNode	!= node->m_right) {
			if (child) {
				child->m_parent = endNode->m_parent;
			}
			endNode->m_parent->m_left = child;
			endNode->m_right = node->m_right;
			node->m_right->m_parent = endNode;
			endNodeParent = endNode->m_parent;
		}


		if (node == *head) {
			*head = endNode;
		} else if (node == node->m_parent->m_left) {
			node->m_parent->m_left = endNode;
		} else {
			node->m_parent->m_right = endNode;
		}
		endNode->m_parent = node->m_parent;

		bool oldColor = endNode->GetColor();
		endNode->SetColor (node->GetColor());
		node->SetColor	(oldColor);

		if (oldColor == BLACK) {
			endNodeParent->RemoveFixup (child, head);
		}
	}
}



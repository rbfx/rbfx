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


#ifndef __D_MAP__
#define __D_MAP__

template<class OBJECT, class KEY>
class dMap
{
	public:
	class dTreeNode
	{
		enum REDBLACK_COLOR
		{
			RED = true,
			BLACK = false
		};

		dTreeNode (const KEY &key, dTreeNode* parentNode)
			:m_info()
			,m_key(key)
			,m_left(nullptr)
			,m_right(nullptr)
			,m_parent(parentNode)
			,m_color(false)
			,m_inTree(false)
		{
			SetColor(RED);
			SetInTreeFlag(true);
		}
	
		dTreeNode (const OBJECT &info, const KEY &key, dTreeNode* parentNode)
			:m_info(info)
			,m_key(key)
			,m_left(nullptr)
			,m_right(nullptr)
			,m_parent(parentNode)
			,m_color(false)
			,m_inTree(false)
		{
			SetColor(RED);
			SetInTreeFlag(true);
		}

		virtual ~dTreeNode () 
		{
		}

		void SetInTreeFlag(bool flag)
		{
			m_inTree = flag;
		}

		bool GetColor() const
		{
			return m_color;
		}

		void SetColor(bool color)
		{
			m_color = color;
		}

		dTreeNode& operator= (dTreeNode& src)
		{
			dAssert (0);
			return* this;
		}

		dTreeNode* GetLeft () const
		{
			return m_left;
		}

		dTreeNode* GetRight () const
		{
			return m_right;
		}

		dTreeNode* GetParent ()
		{
			return m_parent;
		}

		void SetLeft (dTreeNode* const node)
		{
			m_left = node;
		}

		void SetRight (dTreeNode* const node)
		{
			m_right = node;
		}

		void SetParent (dTreeNode* const node)
		{
			m_parent = node;
		}

		public:
		const KEY& GetKey() const
		{
			return m_key;
		}

		OBJECT& GetInfo()
		{
			return m_info;
		}

		dTreeNode* Minimum() const
		{
			dTreeNode* ptr = (dTreeNode*)this;
			for (; ptr->m_left; ptr = ptr->m_left);
			return ptr;
		}

		dTreeNode* Next() const
		{
			if (m_right) {
				return m_right->Minimum();
			}

			dTreeNode* node = (dTreeNode*)this;
			dTreeNode* ptr = m_parent;
			for (; ptr && node == ptr->m_right; ptr = ptr->m_parent) {
				node = ptr;
			}
			return ptr;
		}

		void Unlink(dTreeNode** const head)
		{
			dTreeNode* const node = this;
			node->SetInTreeFlag(false);

			if (!node->m_left || !node->m_right) {
				// y has a nullptr node as a child 
				dTreeNode* const endNode = node;

				// x is y's only child 
				dTreeNode* child = endNode->m_right;
				if (endNode->m_left) {
					child = endNode->m_left;
				}

				// remove y from the parent chain 
				if (child) {
					child->m_parent = endNode->m_parent;
				}

				if (endNode->m_parent) {
					if (endNode == endNode->m_parent->m_left) {
						endNode->m_parent->m_left = child;
					} else {
						endNode->m_parent->m_right = child;
					}
				} else {
					*head = child;
				}

				if (endNode->GetColor() == BLACK) {
					endNode->m_parent->RemoveFixup(child, head);
				}
			} else {

				// find tree successor with a nullptr node as a child 
				dTreeNode* endNode = node->m_right;
				while (endNode->m_left != nullptr) {
					endNode = endNode->m_left;
				}

				// x is y's only child 
				dTreeNode* const child = endNode->m_right;

				endNode->m_left = node->m_left;
				node->m_left->m_parent = endNode;

				dTreeNode* endNodeParent = endNode;
				if (endNode != node->m_right) {
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
				endNode->SetColor(node->GetColor());
				node->SetColor(oldColor);

				if (oldColor == BLACK) {
					endNodeParent->RemoveFixup(child, head);
				}
			}
		}

		void RotateLeft(dTreeNode** const head)
		{
			dTreeNode* const me = this;
			dTreeNode* const child = me->m_right;

			//dAssert(child);
			me->m_right = child->m_left;
			if (child->m_left != nullptr) {
				child->m_left->m_parent = me;
			}

			if (child != nullptr) {
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
			if (me != nullptr) {
				me->m_parent = child;
			}
		}

		// rotate node me to right  *
		void RotateRight(dTreeNode** const head)
		{
			dTreeNode* const me = this;
			dTreeNode* const child = me->m_left;

			//dAssert(child);
			me->m_left = child->m_right;
			if (child->m_right != nullptr) {
				child->m_right->m_parent = me;
			}

			// establish child->m_parent link 
			if (child != nullptr) {
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
			if (me != nullptr) {
				me->m_parent = child;
			}
		}

		void RemoveFixup(dTreeNode* const me, dTreeNode** const head)
		{
			dTreeNode* ptr = this;
			dTreeNode* node = me;
			while ((node != *head) && (!node || node->GetColor() == BLACK)) {
				if (node == ptr->m_left) {
					if (!ptr) {
						return;
					}
					dTreeNode* tmp = ptr->m_right;
					if (!tmp) {
						return;
					}
					if (tmp->GetColor() == RED) {
						tmp->SetColor(BLACK);
						ptr->SetColor(RED);
						ptr->RotateLeft(head);
						tmp = ptr->m_right;
						if (!tmp) {
							return;
						}
					}
					if ((!tmp->m_left || (tmp->m_left->GetColor() == BLACK)) &&
						(!tmp->m_right || (tmp->m_right->GetColor() == BLACK))) {
						tmp->SetColor(RED);
						node = ptr;
						ptr = ptr->m_parent;
						continue;
					} else if (!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) {
						tmp->m_left->SetColor(BLACK);
						tmp->SetColor(RED);
						tmp->RotateRight(head);
						tmp = ptr->m_right;
						//if (!ptr || !tmp) {
						if (!tmp) {
							return;
						}
					}
					tmp->SetColor(ptr->GetColor());
					if (tmp->m_right) {
						tmp->m_right->SetColor(BLACK);
					}
					if (ptr) {
						ptr->SetColor(BLACK);
						ptr->RotateLeft(head);
					}
					node = *head;

				} else {
					if (!ptr) {
						return;
					}
					dTreeNode* tmp = ptr->m_left;
					if (!tmp) {
						return;
					}
					if (tmp->GetColor() == RED) {
						tmp->SetColor(BLACK);
						ptr->SetColor(RED);
						ptr->RotateRight(head);
						tmp = ptr->m_left;
						if (!tmp) {
							return;
						}
					}

					if ((!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) &&
						(!tmp->m_left || (tmp->m_left->GetColor() == BLACK))) {
						tmp->SetColor(RED);
						node = ptr;
						ptr = ptr->m_parent;
						continue;
					} else if (!tmp->m_left || (tmp->m_left->GetColor() == BLACK)) {
						tmp->m_right->SetColor(BLACK);
						tmp->SetColor(RED);
						tmp->RotateLeft(head);
						tmp = ptr->m_left;
						//if (!ptr || !tmp) {
						if (!tmp) {
							return;
						}
					}
					tmp->SetColor(ptr->GetColor());
					if (tmp->m_left) {
						tmp->m_left->SetColor(BLACK);
					}
					if (ptr) {
						ptr->SetColor(BLACK);
						ptr->RotateRight(head);
					}
					node = *head;
				}
			}
			if (node) {
				node->SetColor(BLACK);
			}
		}

		void InsertFixup(dTreeNode** const head)
		{
			dTreeNode* ptr = this;
			// check Red-Black properties 
			//dAssert((ptr == *head) || ptr->m_parent);
			while ((ptr != *head) && (ptr->m_parent->GetColor() == RED)) {
				// we have a violation 
				//dAssert(ptr->m_parent);
				//dAssert(ptr->m_parent->m_parent);
				if (ptr->m_parent == ptr->m_parent->m_parent->m_left) {
					dTreeNode* const tmp = ptr->m_parent->m_parent->m_right;
					if (tmp && (tmp->GetColor() == RED)) {
						// uncle is RED 
						ptr->m_parent->SetColor(BLACK);
						tmp->SetColor(BLACK);
						ptr->m_parent->m_parent->SetColor(RED);
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
					dTreeNode* const tmp = ptr->m_parent->m_parent->m_left;
					if (tmp && (tmp->GetColor() == RED)) {
						//uncle is RED 
						ptr->m_parent->SetColor(BLACK);
						tmp->SetColor(BLACK);
						ptr->m_parent->m_parent->SetColor(RED);
						ptr = ptr->m_parent->m_parent;
					} else {
						// uncle is BLACK 
						if (ptr == ptr->m_parent->m_left) {
							ptr = ptr->m_parent;
							ptr->RotateRight(head);
						}
						ptr->m_parent->SetColor(BLACK);
						if (ptr->m_parent->m_parent->GetColor() == BLACK) {
							ptr->m_parent->m_parent->SetColor(RED);
							ptr->m_parent->m_parent->RotateLeft(head);
						}
					}
				}
			}
			(*head)->SetColor(BLACK);
		}


		private:
		OBJECT m_info;
		KEY m_key; 
		dTreeNode* m_left;
		dTreeNode* m_right;
		dTreeNode* m_parent;
		bool m_color;
		bool m_inTree;
		friend class dMap<OBJECT, KEY>;
	};

	class Iterator
	{
		public:
		Iterator(const dMap<OBJECT,KEY> &me)
		{
			m_ptr = nullptr;
			m_tree = &me;
		}

		~Iterator()
		{
		}

		void Begin() 
		{
			m_ptr = m_tree->Minimum();
		}

		void End()  
		{
			m_ptr = m_tree->Maximum();
		}

		void Set (dTreeNode* const node)
		{
			m_ptr = node;
		}

		operator int() const 
		{
			return m_ptr != nullptr;
		}

		void operator++ ()
		{
			//dAssert (m_ptr);
			m_ptr = m_ptr->Next();
		}

		void operator++ (int)
		{
			//dAssert (m_ptr);
			m_ptr = m_ptr->Next();
		}

		void operator-- () 
		{
			//dAssert (m_ptr);
			m_ptr = m_ptr->Prev();
		}

		void operator-- (int) 
		{
			//dAssert (m_ptr);
			m_ptr = m_ptr->Prev();
		}

		OBJECT &operator* () const 
		{
			return ((dTreeNode*)m_ptr)->GetInfo();
		}

		dTreeNode* GetNode() const
		{
			return (dTreeNode*)m_ptr;
		}

		KEY GetKey () const
		{
			dTreeNode* const tmp = (dTreeNode*)m_ptr;
			return tmp ? tmp->GetKey() : KEY(0);
		}

		private:
		dTreeNode* m_ptr;
		const dMap* m_tree;
	};


	// ***********************************************************
	// member functions
	// ***********************************************************
	public:
	dMap ();
	virtual ~dMap (); 

	operator int() const;
	int GetCount() const;

	dTreeNode* GetRoot () const;
	dTreeNode* Minimum () const;
	dTreeNode* Maximum () const;

	dTreeNode* Find (KEY key) const;
	dTreeNode* FindGreater (KEY key) const;
	dTreeNode* FindGreaterEqual (KEY key) const;
	dTreeNode* FindLessEqual (KEY key) const;

	dTreeNode* GetNodeFromInfo (OBJECT &info) const;

	dTreeNode* Insert (KEY key);
	dTreeNode* Insert (const OBJECT &element, KEY key);
	dTreeNode* Insert (const OBJECT &element, KEY key, bool& elementWasInTree);
	dTreeNode* Insert (dTreeNode* const node, KEY key);

	dTreeNode* Replace (OBJECT &element, KEY key);
	dTreeNode* ReplaceKey (KEY oldKey, KEY newKey);
	dTreeNode* ReplaceKey (dTreeNode* const node, KEY key);

	void Unlink (dTreeNode* const node);

	void Remove (KEY key);
	void Remove (dTreeNode* const node);
	void RemoveAll (); 

	bool SanityCheck () const;


	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
	void RemoveAllLow (dTreeNode* const root);
	int CompareKeys (const KEY &key0, const KEY &key1) const;
	bool SanityCheck (dTreeNode* const ptr, int height) const;

	int m_count;
	dTreeNode* m_head;
	friend class dTreeNode;
};

template<class OBJECT, class KEY>
dMap<OBJECT, KEY>::dMap ()
{
	m_count	= 0;
	m_head = nullptr;
}

template<class OBJECT, class KEY>
dMap<OBJECT, KEY>::~dMap () 
{
	RemoveAll();
}


template<class OBJECT, class KEY>
dMap<OBJECT, KEY>::operator int() const
{
	return m_head != nullptr;
}

template<class OBJECT, class KEY>
int dMap<OBJECT, KEY>::GetCount() const
{
	return m_count;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Minimum () const
{
	return m_head ? (dTreeNode*) m_head->Minimum() : nullptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Maximum () const
{
	return m_head ? (dTreeNode*) m_head->Maximum() : nullptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::GetRoot () const
{
	return m_head;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Find (KEY key) const
{
	if (m_head == nullptr) {
		return nullptr;
	}

	dTreeNode* ptr = m_head;
	while (ptr != nullptr) {
		int val = CompareKeys (ptr->m_key, key);
		if (!val) {
			break;
		}
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}
	return ptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::GetNodeFromInfo (OBJECT &info) const
{
	dTreeNode* node = (dTreeNode*) &info;
	int offset = ((char*) &node->m_info) - ((char *) node);
	node = (dTreeNode*) (((char *) node) - offset);

//	dAssert (node->IsInTree ());
	dAssert (&node->GetInfo () == &info);
	return (node->IsInTree ()) ? node : nullptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::FindGreater (KEY key) const
{
	if (m_head == nullptr) {
		return nullptr;
	}

	dTreeNode* prev = nullptr;
	dTreeNode* ptr = m_head;
	int val = 0;
	while (ptr != nullptr) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			return (dTreeNode*) ptr->Next();
		}
		prev = ptr;
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	if (val > 0) {
		while (prev->m_parent && (prev->m_parent->m_right == prev)) {
			prev = prev->GetParent(); 
		}
		prev = prev->GetParent(); 
	}
	return (dTreeNode*) prev; 
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::FindGreaterEqual (KEY key) const
{
	if (m_head == nullptr) {
		return nullptr;
	}

	dTreeNode* prev = nullptr;
	dTreeNode* ptr = m_head;
	int val = 0;
	while (ptr != nullptr) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			return ptr;
		}
		prev = ptr;
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	if (val > 0) {
		while (prev->m_parent && (prev->m_parent->m_right == prev)) {
			prev = prev->GetParent(); 
		}
		prev = prev->GetParent(); 
	}
	return (dTreeNode*) prev; 
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::FindLessEqual (KEY key) const
{
	if (m_head == nullptr) {
		return nullptr;
	}

	dTreeNode* prev = nullptr;
	dTreeNode* ptr = m_head;
	int val = 0;
	while (ptr != nullptr) {
		val = CompareKeys (ptr->m_key, key);
		if (!val) {
			return ptr;
		}
		prev = ptr;
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	if (val < 0) {
		while (prev->m_parent && (prev->m_parent->m_left == prev)) {
			prev = prev->GetParent(); 
		}
		prev = prev->GetParent(); 
	}
	return (dTreeNode*) prev; 
}


template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Insert (KEY key)
{
	dTreeNode* parent = nullptr;
	dTreeNode* ptr = m_head;
	int val = 0;
	while (ptr != nullptr) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);

		if (val < 0) {
			ptr = ptr->GetLeft();
		} else if (val > 0) {
			ptr = ptr->GetRight();
		} else {
			return ptr;
		}
	}
	m_count	++;

	ptr = new dTreeNode (key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dTreeNode**)&m_head);
	return ptr;
}


template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Insert (const OBJECT &element, KEY key, bool& elementWasInTree)
{
	dTreeNode* parent = nullptr;
	dTreeNode* ptr = m_head;
	int val = 0;
	elementWasInTree = false;
	while (ptr != nullptr) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);

		if (val < 0) {
			ptr = ptr->GetLeft();
		} else if (val > 0) {
			ptr = ptr->GetRight();
		} else {
			elementWasInTree = true;
			return ptr;
		}
	}
	m_count	++;

	ptr = new dTreeNode (element, key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dTreeNode**)&m_head);
	return ptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Insert (const OBJECT &element, KEY key)
{
	bool foundState;
	dTreeNode* node = Insert (element, key, foundState);
	if (foundState) {
		node = nullptr;
	}
	return node;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Insert (typename dMap<OBJECT, KEY>::dTreeNode* const node, KEY key)
{
	int val = 0;
	dTreeNode* ptr = m_head;
	dTreeNode* parent = nullptr;
	while (ptr != nullptr) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);

		if (val < 0) {
			ptr = ptr->GetLeft();
		} else if (val > 0) {
			ptr = ptr->GetRight();
		} else {
			return nullptr;
		}
	}

	m_count	++;

	ptr = node;
	ptr->m_key = key;
	ptr->Initdata (parent);

	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dTreeNode**)&m_head);
	return ptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::Replace (OBJECT &element, KEY key)
{
	dTreeNode* parent = nullptr;
	dTreeNode* ptr = m_head;
	int val = 0;
	while (ptr != nullptr) {
		parent = ptr;
		val = CompareKeys (ptr->m_key, key);
		if (val == 0) {
			ptr->m_info = element;
			return ptr;
		}
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	ptr = new dTreeNode (element, key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}
	ptr->InsertFixup ((dTreeNode**)&m_head);
	return ptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::ReplaceKey (typename dMap<OBJECT, KEY>::dTreeNode* node, KEY key)
{
	Unlink(node);
	dTreeNode* const ptr = Insert (node, key);

	dAssert (ptr);
	return ptr;
}

template<class OBJECT, class KEY>
typename dMap<OBJECT, KEY>::dTreeNode* dMap<OBJECT, KEY>::ReplaceKey (KEY oldKey, KEY newKey)
{
	dTreeNode* const node = Find (oldKey);
	return node ? ReplaceKey (node, newKey) : nullptr;
}

template<class OBJECT, class KEY>
void dMap<OBJECT, KEY>::Unlink (typename dMap<OBJECT, KEY>::dTreeNode* const node)
{
	m_count	--;
	node->Unlink((dTreeNode** )&m_head);
}

template<class OBJECT, class KEY>
void dMap<OBJECT, KEY>::Remove (typename dMap<OBJECT, KEY>::dTreeNode* const node)
{
	m_count	--;
	node->Unlink ((dTreeNode** )&m_head);
	delete node;
}

template<class OBJECT, class KEY>
void dMap<OBJECT, KEY>::Remove (KEY key) 
{
	// find node in tree 
	dTreeNode* const node = Find (key);
	if (node) {
		Remove (node);
	}
}


template<class OBJECT, class KEY>
void dMap<OBJECT, KEY>::RemoveAllLow (dTreeNode* const root) 
{
	if (root->m_left) {
		RemoveAllLow((dTreeNode*)root->m_left);
	}
	if (root->m_right) {
		RemoveAllLow ((dTreeNode*)root->m_right);
	}
	root->SetInTreeFlag(false);
	delete root;
}


template<class OBJECT, class KEY>
void dMap<OBJECT, KEY>::RemoveAll () 
{
	if (m_head) {
		m_count	 = 0;
		dTreeNode* root;
		for (root = m_head; root->m_parent; root = (dTreeNode*)root->m_parent);
		RemoveAllLow(root);
		m_head = nullptr;
	}
}

template<class OBJECT, class KEY>
bool dMap<OBJECT, KEY>::SanityCheck () const
{
	return SanityCheck (m_head, 0);
}


template<class OBJECT, class KEY>
bool dMap<OBJECT, KEY>::SanityCheck (typename dMap<OBJECT, KEY>::dTreeNode* ptr, int height) const
{
	if (!ptr) {
		return true;
	}

	if (ptr->m_left) {
		if (CompareKeys (ptr->m_key, ptr->GetLeft()->m_key) > 0) {
			return false;
		}
	}

	if (ptr->m_right) {
		if (CompareKeys (ptr->m_key, ptr->GetRight()->m_key) < 0) {
			return false;
		}
	}

	if (ptr->GetColor() == dTreeNode::BLACK) {
		height ++;
	} else if (!((!ptr->m_left  || (ptr->m_left->GetColor() == dTreeNode::BLACK)) &&
		       (!ptr->m_right || (ptr->m_right->GetColor() == dTreeNode::BLACK)))) {
	  	return false;
	}

	if (!ptr->m_left && !ptr->m_right) {
		int bh = 0;
		for (dTreeNode* x = ptr; x; x = x->GetParent()) {
	 		if (x->GetColor() == dTreeNode::BLACK) {
				bh ++;
			}
		}
		if (bh != height) {
			return false;
		}
	}

	if (ptr->m_left && !SanityCheck (ptr->GetLeft(), height)) {
		return false;
	}

	if (ptr->m_right && !SanityCheck (ptr->GetRight(), height)) {
		return false;
	}
	return true;
}

template<class OBJECT, class KEY>
int dMap<OBJECT, KEY>::CompareKeys (const KEY &key0, const KEY &key1) const
{
	if (key1 < key0) {
		return - 1;
	}
	if (key1 > key0) {
		return 1;
	}
	return 0;
}

#endif



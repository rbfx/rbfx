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


#ifndef __dList__
#define __dList__

#include "dContainersStdAfx.h"
#include "dContainersAlloc.h"


// a small double link list container similar to STL, 
template<class T, int poolSize = D_MAX_ENTRIES_IN_FREELIST>
class dList: public dContainersAlloc
{
	public:
	class dListNode
	{
		dListNode (dListNode* const prev, dListNode* const next) 
			:m_info () 
		{
			m_prev = prev;
			m_next = next;
			if (m_prev) {
				m_prev->m_next = this;
			}
			if (m_next) {
				m_next->m_prev = this;
			}
		}

		dListNode (const T &info, dListNode* prev, dListNode* next) 
			:m_info (info) 
		{
			m_prev = prev;
			m_next = next;
			if (m_prev) {
				m_prev->m_next = this;
			}
			if (m_next) {
				m_next->m_prev = this;
			}
		}

		virtual ~dListNode()
		{
		}

		void Unlink ()
		{
			if (m_prev) {
				m_prev->m_next = m_next;
			}

			if (m_next) {
				m_next->m_prev = m_prev;
			}
			m_prev = NULL;
			m_next = NULL;
		}

		void Remove()
		{
			Unlink();
			this->~dListNode();
		}

		void AddLast(dListNode* const node) 
		{
			m_next = node;
			node->m_prev = this;
		}

		void AddFirst(dListNode* const node) 
		{
			m_prev = node;
			node->m_next = this;
		}

		public:
		T& GetInfo()
		{
			return m_info;
		}

		const T& GetInfo() const
		{
			return m_info;
		}


		dListNode* GetNext() const
		{
			return m_next;
		}

		dListNode* GetPrev() const
		{
			return m_prev;
		}

		private:
		T m_info;
		dListNode* m_next;
		dListNode* m_prev;
		friend class dList<T, poolSize>;
	};

	class Iterator
	{
		public:
		Iterator (const dList<T, poolSize> &me)
		{
			m_ptr = NULL;
			m_list = (dList *)&me;
		}

		~Iterator ()
		{
		}

		operator int() const
		{
			return m_ptr != NULL;
		}

		bool operator== (const Iterator &target) const
		{
			return (m_ptr == target.m_ptr) && (m_list == target.m_list);
		}

		void Begin()
		{
			m_ptr = m_list->GetFirst();
		}

		void End()
		{
			m_ptr = m_list->GetLast();
		}

		void Set (dListNode* const node)
		{
			m_ptr = node;
		}

		void operator++ ()
		{
			m_ptr = m_ptr->m_next();
		}

		void operator++ (int)
		{
			m_ptr = m_ptr->GetNext();
		}

		void operator-- () 
		{
			m_ptr = m_ptr->GetPrev();
		}

		void operator-- (int) 
		{
			m_ptr = m_ptr->GetPrev();
		}

		T &operator* () const
		{
			return m_ptr->GetInfo();
		}

		dListNode* GetNode() const
		{
			return m_ptr;
		}

		private:
		dList *m_list;
		dListNode* m_ptr;
	};

	// ***********************************************************
	// member functions
	// ***********************************************************
	public:
	dList ();
	virtual ~dList ();

	operator int() const;
	int GetCount() const;
	dListNode* GetLast() const;
	dListNode* GetFirst() const;
	dListNode* Append ();
	dListNode* Append (const T &element);
	dListNode* Addtop ();
	dListNode* Addtop (const T &element);
	dListNode* AppendAfter (dListNode* const node);

	void RotateToEnd (dListNode* const node);
	void RotateToBegin (dListNode* const node);
	void InsertAfter (dListNode* const root, dListNode* const node);

	dListNode* Find (const T &element) const;
	dListNode* GetNodeFromInfo (T &m_info) const;
	void Remove (dListNode* const node);
	void Remove (const T &element);
	void RemoveAll ();
	
	// special routines
	// move the data to the target list and set to zero m_count, m_first and m_last
	void TranferDataToTarget (dList& target);

	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
/*
	bool Sanity() const
	{
		int count = 0;
		for (dListNode* ptr = GetFirst(); ptr; ptr = ptr->GetNext())
			count ++;
		return count == m_count;
	}
*/
	bool Sanity() const {return true;}
	dContainerFixSizeAllocator& GetAllocator()
	{
		static dContainerFixSizeAllocator* allocator = NULL;
		if (!allocator) {
			allocator = dContainerFixSizeAllocator::Create (sizeof (dList<T, poolSize>::dListNode), poolSize);
		}
		return *allocator;
	}

	int m_count;
	dListNode* m_first;
	dListNode* m_last;
	friend class dListNode;
};


template<class T, int poolSize>
dList<T, poolSize>::dList ()
{
	m_count = 0;
	m_first = NULL;
	m_last = NULL;
	GetAllocator();
}


template<class T, int poolSize>
dList<T, poolSize>::~dList () 
{
	RemoveAll ();
}


template<class T, int poolSize>
int dList<T, poolSize>::GetCount() const
{
	return m_count;
}

template<class T, int poolSize>
dList<T, poolSize>::operator int() const
{
	return m_first != NULL;
}

template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::GetFirst() const
{
	return m_first;
}

template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::GetLast() const
{
	return m_last;
}

template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::Append ()
{
	m_count	++;
	if (m_first == NULL) {
		m_first = new (GetAllocator().Alloc()) dListNode(NULL, NULL);
		m_last = m_first;
	} else {
		m_last = new (GetAllocator().Alloc()) dListNode(m_last, NULL);
	}
	dAssert (Sanity());
	return m_last;
}

template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::AppendAfter (dListNode* const node)
{
	dListNode* const ptr = Append ();
	InsertAfter (node, ptr);
	return ptr;
}

template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::Append (const T &element)
{
	m_count	++;
	if (m_first == NULL) {
		m_first = new (GetAllocator().Alloc()) dListNode(element, NULL, NULL);
		m_last = m_first;
	} else {
		m_last = new (GetAllocator().Alloc()) dListNode(element, m_last, NULL);
	}
	dAssert (Sanity());
	return m_last;
}

template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::Addtop ()
{
	m_count	++;
	if (m_last == NULL) {
		m_last = new (GetAllocator().Alloc()) dListNode(NULL, NULL);
		m_first = m_last;
	} else {
		m_first = new (GetAllocator().Alloc()) dListNode(NULL, m_first);
	}
	dAssert (Sanity());
	return m_first;
}


template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::Addtop (const T &element)
{
	m_count	++;
	if (m_last == NULL) {
		m_last = new (GetAllocator().Alloc()) dListNode(element, NULL, NULL);
		m_first = m_last;
	} else {
		m_first = new (GetAllocator().Alloc()) dListNode(element, NULL, m_first);
	}
	dAssert (Sanity());
	return m_first;
}

template<class T, int poolSize>
void dList<T, poolSize>::InsertAfter (dListNode* const root, dListNode* const node)
{
	dAssert (root != node);
	if (node == m_last) {
		m_last = m_last->GetPrev(); 
	}
	node->Unlink();

	node->m_prev = root;
	node->m_next = root->m_next;
	if (root->m_next) {
		root->m_next->m_prev = node;
	} 
	root->m_next = node;

	if (root == m_last) {
		m_last = node;
	}

	dAssert (Sanity());
}

template<class T, int poolSize>
void dList<T, poolSize>::RotateToEnd (dListNode* const node)
{
	if (node != m_last) {
		if (m_last != m_first) {
			if (node == m_first) {
				m_first = m_first->GetNext();
			}
			node->Unlink();
			m_last->AddLast(node);
			m_last = node; 
		}
	}
}

template<class T, int poolSize>
void dList<T, poolSize>::RotateToBegin (dListNode* const node)
{
	if (node != m_first) {
		if (m_last != m_first) {
			if (node == m_last) {
				m_last = m_last->GetPrev();
			}
			node->Unlink();
			m_first->AddFirst(node);
			m_first = node; 
		}
	}
}


template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::Find (const T &element) const
{
	dListNode* node;
	for (node = m_first; node; node = node->GetNext()) {
		if (element	== node->m_info) {
			break;
		}
	}
	return node;
}


template<class T, int poolSize>
typename dList<T, poolSize>::dListNode* dList<T, poolSize>::GetNodeFromInfo (T &info) const
{
	dListNode* const node = (dListNode*) &info;
	long long offset = ((char*) &node->m_info) - ((char*)node);
	dListNode* const retnode = (dListNode*) (((char *) node) - offset);
	dAssert (&retnode->GetInfo () == &info);
	return retnode;
}


template<class T, int poolSize> 
void dList<T, poolSize>::Remove (const T &element)
{
	dListNode* const node = Find (element);
	if (node) {
		Remove (node);
	}
}

template<class T, int poolSize>
void dList<T, poolSize>::Remove (dListNode* const node)
{
	m_count --;
	if (node == m_first) {
		m_first = m_first->GetNext();
	}
	if (node == m_last) {
		m_last = m_last->GetPrev();
	}
	node->Remove();
	GetAllocator().Free (node);
	dAssert (Sanity());
}

template<class T, int poolSize>
void dList<T, poolSize>::RemoveAll ()
{
	while (m_first) {
		Remove(m_first);
	}
	dAssert (!m_count);
}

template<class T, int poolSize>
void dList<T, poolSize>::TranferDataToTarget (dList& target)
{
	dAssert (target.m_count == 0);
	target.m_count = m_count;
	target.m_first = m_first;
	target.m_last = m_last;

	m_count = 0;
	m_first = NULL;
	m_last = NULL;
}

#endif



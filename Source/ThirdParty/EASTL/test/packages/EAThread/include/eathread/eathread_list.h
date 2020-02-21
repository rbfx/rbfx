///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This is a small templated list implementation which suffices for our 
// purposes but is not optimal. It is present in order to avoid dependencies
// on external libraries.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_LIST_H
#define EATHREAD_EATHREAD_LIST_H


#include <eathread/internal/config.h>
#include <eathread/eathread.h>
#include <stddef.h> // size_t, etc.
#include <new>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace EA
{
	namespace Thread
	{

		namespace details
		{
			/// Default allocator implementation used by the simple_list class
			template<typename T>
			struct ListDefaultAllocatorImpl
			{
				template<typename OT>
				struct rebind { typedef ListDefaultAllocatorImpl<OT> other; };

				T* construct()
				{
					Allocator* pAllocator = GetAllocator();

					if(pAllocator)
						return new(pAllocator->Alloc(sizeof(T))) T;
					else
						return new T;
				}

				void destroy(T* obj)
				{
					Allocator* pAllocator = GetAllocator();

					if(pAllocator)
					{
						obj->~T();
						pAllocator->Free(obj);
					}
					else
						delete obj;
				}
			};
		}


		/// Simple version of an STL bidirectional list.
		/// Implemented to avoid dependency on container implementations.
		///
		/// This implementation has some non-stl standard methods like find. 
		///            
		template<typename T, class Allocator = details::ListDefaultAllocatorImpl<T> >
		class simple_list
		{
			simple_list(const simple_list&);
			simple_list& operator=(const simple_list&);

		protected:
			struct list_node
			{
				T          mValue;
				list_node* mpPrev;
				list_node* mpNext;
			};

			typedef list_node node_t;
			typedef typename  Allocator::template rebind<list_node>::other allocator_t;
			
			allocator_t      mAllocator;
			node_t*          mpNodeHead;
			node_t*          mpNodeTail;
			size_t           mnSize;

		public:
			typedef T        value_type;              //< list value type
			typedef const T  const_value_type;        //< constant list value type
			typedef const T& const_value_ref_type;    //< constant reference list value type
			
			struct         const_iterator;
			struct         iterator;
			friend  struct const_iterator;
			friend  struct iterator;


			struct const_iterator
			{
				friend class simple_list<T>;

				const_iterator()
					: mpNode(NULL)
				{ }

				const_iterator(const const_iterator& rhs)
					: mpNode(rhs.mpNode)
				{ }

				const_iterator& operator=(const const_iterator& rhs)
				{
					mpNode = rhs.mpNode;
					return *this;
				}

				const T& operator*() const
					{ return mpNode->mValue; }

				const T* operator->() const
					{ return &**this; }  
					 
				bool operator==(const const_iterator& rhs) const
					{ return rhs.mpNode == mpNode; }

				bool operator!=(const const_iterator& rhs) const
					{ return rhs.mpNode != mpNode; }

				const_iterator& operator++()
				{
					mpNode = mpNode->mpNext;
					return *this;
				}

			protected:
				const node_t* mpNode;

			protected:
				const_iterator(node_t* pNode)
					: mpNode(pNode)
				{ }

				const_iterator& operator=(const node_t* pNode)
				{
					mpNode = pNode;
					return *this;
				}
			}; // const_iterator



			struct iterator : public const_iterator
			{
				friend class simple_list<T>;

				iterator()
					: const_iterator(){ }

				iterator(const const_iterator& rhs)
					: const_iterator(rhs)
				{ }

				iterator& operator=(const const_iterator& rhs)
				{
					*static_cast<const_iterator*>(this)= rhs;
					return *this;
				}

				T& operator*() const
					{ return const_cast<T&>(**static_cast<const const_iterator*>(this)); }

				T& operator->() const
					{ return const_cast<T*>(&**static_cast<const const_iterator*>(this)); }

				iterator& operator++()
				{
					++(*static_cast<const_iterator*>(this));
					return *this;
				}

			protected:
				iterator(node_t* pNode)
					: const_iterator(pNode)
				{ }

				iterator& operator=(node_t* pNode)
				{
					const_cast<node_t*>(*this) = pNode;
					return *this;
				}
			}; // iterator



			simple_list()
				: mnSize(0)
			{
				mpNodeHead         = mAllocator.construct();
				mpNodeTail         = mAllocator.construct();
				mpNodeHead->mpNext = mpNodeTail;
				mpNodeHead->mpPrev = mpNodeTail;
				mpNodeTail->mpNext = mpNodeHead;
				mpNodeTail->mpPrev = mpNodeHead;
			}

			~simple_list()
			{
				clear();
				mAllocator.destroy(mpNodeHead);
				mAllocator.destroy(mpNodeTail);
			}

			bool empty() const
				{ return mpNodeHead->mpNext == mpNodeTail; }

			void push_back(const T& value)
			{
				node_t* const pNode   = mAllocator.construct();
				pNode->mValue         = value;
				pNode->mpPrev         = mpNodeTail->mpPrev;                        
				pNode->mpNext         = mpNodeTail;
				pNode->mpPrev->mpNext = pNode;
				mpNodeTail->mpPrev    = pNode;
				++mnSize;
			}

			void push_front(const T& value)
			{
				node_t* const pNode = mAllocator.construct();
				pNode->mValue       = value;
				pNode->mpPrev       = mpNodeHead;
				pNode->mpNext       = mpNodeHead->mpNext;
				mpNodeHead->mpNext  = pNode;
				++mnSize;
			}

			void pop_front()
			{
				if(!empty())
				{
					node_t* const pNode   = mpNodeHead->mpNext;
					mpNodeHead->mpNext    = pNode->mpNext;
					pNode->mpNext->mpPrev = mpNodeHead;
					mAllocator.destroy(pNode);
					--mnSize;
				}
			}

			size_t size() const
				{ return mnSize; }

			iterator erase(iterator& iter)
			{
				if(!empty())
				{
					node_t* const pNext = iter.mpNode->mpNext;
					iter.mpNode->mpNext->mpPrev = iter.mpNode->mpPrev;
					iter.mpNode->mpPrev->mpNext = iter.mpNode->mpNext;
					--mnSize;
					mAllocator.destroy(const_cast<node_t*>(iter.mpNode));
					return pNext;
				}
				return end();
			}

			void clear()
			{
				if(!empty())
				{
					node_t* pNode = mpNodeHead->mpNext;

					while(pNode != mpNodeTail)
					{
						node_t* const pNext   = pNode->mpNext;
						pNode->mpNext->mpPrev = pNode->mpPrev;
						pNode->mpPrev->mpNext = pNext;
						mAllocator.destroy(pNode);
						pNode = pNext;
					}
					mnSize = 0;
				}
			}

			T& front() const
				{ return mpNodeHead->mpNext->mValue; }

			const const_iterator begin() const
				{ return mpNodeHead->mpNext; }

			const const_iterator end() const
				{ return mpNodeTail; }

			/// returns end()if not found
			iterator find(const T& element)
			{
				iterator iter = begin();
				while((iter != end()) && !(element == *iter))
					 ++iter;
				return iter;
			}

		}; // simple_list

	} // namespace Thread

} // namespace EA


#endif // EATHREAD_EATHREAD_LIST_H









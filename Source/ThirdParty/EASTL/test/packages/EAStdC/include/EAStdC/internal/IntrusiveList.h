///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This is a curtailed version of eastl::intrusive_list.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_INTRUSIVELIST_H
#define EASTDC_INTRUSIVELIST_H


#include <EABase/eabase.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
EA_RESTORE_ALL_VC_WARNINGS()


// Disabled because we want to remove our dependence on STL.
//
//#ifdef _MSC_VER
//    #pragma warning(push, 0)
//#endif
//#include <iterator>
//#ifdef _MSC_VER
//    #pragma warning(pop)
//#endif


namespace EA
{
	namespace StdC
	{

		template <typename Category, typename T, typename Distance = ptrdiff_t, 
				  typename Pointer = T*, typename Reference = T&>
		struct iterator
		{
			typedef Category  iterator_category;
			typedef T         value_type;
			typedef Distance  difference_type;
			typedef Pointer   pointer;
			typedef Reference reference;
		};


		/* Disabled because we want to remove our dependence on STL.

		// struct iterator_traits
		template <typename Iterator>
		struct iterator_traits
		{
			typedef typename Iterator::iterator_category iterator_category;
			typedef typename Iterator::value_type        value_type;
			typedef typename Iterator::difference_type   difference_type;
			typedef typename Iterator::pointer           pointer;
			typedef typename Iterator::reference         reference;
		};

		template <typename T>
		struct iterator_traits<T*>
		{
			typedef std::random_access_iterator_tag          iterator_category;
			typedef T                                        value_type;
			typedef ptrdiff_t                                difference_type;
			typedef T*                                       pointer;
			typedef T&                                       reference;
		};

		template <typename T>
		struct iterator_traits<const T*>
		{
			typedef std::random_access_iterator_tag          iterator_category;
			typedef T                                        value_type;
			typedef ptrdiff_t                                difference_type;
			typedef const T*                                 pointer;
			typedef const T&                                 reference;
		};


		/// reverse_iterator
		///
		/// From the C++ standard:
		/// Bidirectional and random access iterators have corresponding reverse 
		/// iterator adaptors that iterate through the data structure in the 
		/// opposite direction. They have the same signatures as the corresponding 
		/// iterators. The fundamental relation between a reverse iterator and its 
		/// corresponding iterator i is established by the identity:
		///     &*(reverse_iterator(i)) == &*(i - 1).
		/// This mapping is dictated by the fact that while there is always a pointer 
		/// past the end of an array, there might not be a valid pointer before the
		/// beginning of an array.
		///
		template <typename Iterator>
		class reverse_iterator : public iterator<typename EA::StdC::iterator_traits<Iterator>::iterator_category,
												 typename EA::StdC::iterator_traits<Iterator>::value_type,
												 typename EA::StdC::iterator_traits<Iterator>::difference_type,
												 typename EA::StdC::iterator_traits<Iterator>::pointer,
												 typename EA::StdC::iterator_traits<Iterator>::reference>
		{
		public:
			typedef Iterator                                                      iterator_type;
			typedef typename EA::StdC::iterator_traits<Iterator>::pointer         pointer;
			typedef typename EA::StdC::iterator_traits<Iterator>::reference       reference;
			typedef typename EA::StdC::iterator_traits<Iterator>::difference_type difference_type;

		protected:
			Iterator mIterator;

		public:
			reverse_iterator()      // It's important that we construct mIterator, because if Iterator  
				: mIterator() { }   // is a pointer, there's a difference between doing it and not.

			explicit reverse_iterator(iterator_type i)
				: mIterator(i) { }

			reverse_iterator(const reverse_iterator& ri)
				: mIterator(ri.mIterator) { }

			template <typename U>
			reverse_iterator(const reverse_iterator<U>& ri)
				: mIterator(ri.base()) { }

			// This operator= isn't in the standard, but the the C++ 
			// library working group has tentatively approved it, as it
			// allows const and non-const reverse_iterators to interoperate.
			template <typename U>
			reverse_iterator<Iterator>& operator=(const reverse_iterator<U>& ri)
				{ mIterator = ri.base(); return *this; }

			iterator_type base() const
				{ return mIterator; }

			reference operator*() const
			{
				iterator_type i(mIterator);
				return *--i;
			}

			pointer operator->() const
				{ return &(operator*()); }

			reverse_iterator& operator++()
				{ --mIterator; return *this; }

			reverse_iterator operator++(int)
			{
				reverse_iterator ri(*this);
				--mIterator;
				return ri;
			}

			reverse_iterator& operator--()
				{ ++mIterator; return *this; }

			reverse_iterator operator--(int)
			{
				reverse_iterator ri(*this);
				++mIterator;
				return ri;
			}

			reverse_iterator operator+(difference_type n) const
				{ return reverse_iterator(mIterator - n); }

			reverse_iterator& operator+=(difference_type n)
				{ mIterator -= n; return *this; }

			reverse_iterator operator-(difference_type n) const
				{ return reverse_iterator(mIterator + n); }

			reverse_iterator& operator-=(difference_type n)
				{ mIterator += n; return *this; }

			reference operator[](difference_type n) const
				{ return mIterator[-n - 1]; } 
		};
		*/


		/// intrusive_list_node
		///
		struct intrusive_list_node
		{
			intrusive_list_node* mpNext;
			intrusive_list_node* mpPrev;
		};


		/// intrusive_list_iterator
		///
		template <typename T, typename Pointer, typename Reference>
		class intrusive_list_iterator
		{
		public:
			typedef intrusive_list_iterator<T, Pointer, Reference>   this_type;
			typedef intrusive_list_iterator<T, T*, T&>               iterator;
			typedef intrusive_list_iterator<T, const T*, const T&>   const_iterator;
			typedef T                                                value_type;
			typedef T                                                node_type;
			typedef ptrdiff_t                                        difference_type;
			typedef Pointer                                          pointer;
			typedef Reference                                        reference;
		  //typedef std::bidirectional_iterator_tag                  iterator_category;

		public:
			pointer mpNode; // Needs to be public for operator==() to work

		public:
			intrusive_list_iterator();
			explicit intrusive_list_iterator(pointer pNode);  // Note that you can also construct an iterator from T via this, since value_type == node_type.
			intrusive_list_iterator(const iterator& x);

			reference operator*() const;
			pointer   operator->() const;

			intrusive_list_iterator& operator++();
			intrusive_list_iterator& operator--();

			intrusive_list_iterator operator++(int);
			intrusive_list_iterator operator--(int);

		}; // class intrusive_list_iterator



		/// intrusive_list_base
		///
		class intrusive_list_base
		{
		public:
			typedef size_t       size_type;     // See config.h for the definition of this, which defaults to uint32_t.
			typedef ptrdiff_t    difference_type;

		protected:
			intrusive_list_node mAnchor;          ///< Sentinel node (end). All data nodes are linked in a ring from this node.

		public:
			intrusive_list_base();

			bool         empty() const;
			size_t       size() const;              ///< Returns the number of elements in the list; O(n).
			void         clear();                   ///< Clears the list; O(1). No deallocation occurs.
			void         pop_front();               ///< Removes an element from the front of the list; O(1). The element must exist, but is not deallocated.
			void         pop_back();                ///< Removes an element from the back of the list; O(1). The element must exist, but is not deallocated.

		}; // class intrusive_list_base



		template <typename T = intrusive_list_node>
		class intrusive_list : public intrusive_list_base
		{
		public:
			typedef intrusive_list<T>                               this_type;
			typedef intrusive_list_base                             base_type;
			typedef T                                               node_type;
			typedef T                                               value_type;
			typedef typename base_type::size_type                   size_type;
			typedef typename base_type::difference_type             difference_type;
			typedef T&                                              reference;
			typedef const T&                                        const_reference;
			typedef T*                                              pointer;
			typedef const T*                                        const_pointer;
			typedef intrusive_list_iterator<T, T*, T&>              iterator;
			typedef intrusive_list_iterator<T, const T*, const T&>  const_iterator;
		  //typedef EA::StdC::reverse_iterator<iterator>            reverse_iterator;
		  //typedef EA::StdC::reverse_iterator<const_iterator>      const_reverse_iterator;

		public:
			intrusive_list();                                ///< Creates an empty list.
			intrusive_list(const this_type& x);              ///< Creates an empty list; ignores the argument.
			this_type& operator=(const this_type& x);        ///< Clears the list; ignores the argument.

			iterator                begin();                 ///< Returns an iterator pointing to the first element in the list.
			const_iterator          begin() const;           ///< Returns a const_iterator pointing to the first element in the list.
			iterator                end();                   ///< Returns an iterator pointing one-after the last element in the list.
			const_iterator          end() const;             ///< Returns a const_iterator pointing one-after the last element in the list.
		  //reverse_iterator        rbegin();                ///< Returns a reverse_iterator pointing at the end of the list (start of the reverse sequence).
		  //const_reverse_iterator  rbegin() const;          ///< Returns a const_reverse_iterator pointing at the end of the list (start of the reverse sequence).
		  //reverse_iterator        rend();                  ///< Returns a reverse_iterator pointing at the start of the list (end of the reverse sequence).
		  //const_reverse_iterator  rend() const;            ///< Returns a const_reverse_iterator pointing at the start of the list (end of the reverse sequence).
			
			reference               front();                 ///< Returns a reference to the first element. The list must be empty.
			const_reference         front() const;           ///< Returns a const reference to the first element. The list must be empty.
			reference               back();                  ///< Returns a reference to the last element. The list must be empty.
			const_reference         back() const;            ///< Returns a const reference to the last element. The list must be empty.

			void        push_front(T& x);                    ///< Adds an element to the front of the list; O(1). The element is not copied. The element must not be in any other list.
			void        push_back(T& x);                     ///< Adds an element to the back of the list; O(1). The element is not copied. The element must not be in any other list.

			bool        contains(const T& x) const;          ///< Returns true if the given element is in the list; O(n). Equivalent to (locate(x) != end()).

			iterator        locate(T& x);                    ///< Converts a reference to an object in the list back to an iterator, or returns end() if it is not part of the list. O(n)
			const_iterator  locate(const T& x) const;        ///< Converts a const reference to an object in the list back to a const iterator, or returns end() if it is not part of the list. O(n)

			iterator    insert(iterator pos, T& x);          ///< Inserts an element before the element pointed to by the iterator. O(1)
			iterator    erase(iterator pos);                 ///< Erases the element pointed to by the iterator. O(1)
			iterator    erase(iterator pos, iterator last);  ///< Erases elements within the iterator range [pos, last). O(1)
			void        swap(intrusive_list&);               ///< Swaps the contents of two intrusive lists; O(1).

			static void remove(T& value);                    ///< Erases an element from a list; O(1). Note that this is static so you don't need to know which list the element, although it must be in some list.

		}; // intrusive_list




		///////////////////////////////////////////////////////////////////////
		// intrusive_list_iterator
		///////////////////////////////////////////////////////////////////////

		template <typename T, typename Pointer, typename Reference>
		inline intrusive_list_iterator<T, Pointer, Reference>::intrusive_list_iterator()
		{
			// Empty
		}


		template <typename T, typename Pointer, typename Reference>
		inline intrusive_list_iterator<T, Pointer, Reference>::intrusive_list_iterator(pointer pNode)
			: mpNode(pNode)
		{
			// Empty
		}


		template <typename T, typename Pointer, typename Reference>
		inline intrusive_list_iterator<T, Pointer, Reference>::intrusive_list_iterator(const iterator& x)
			: mpNode(x.mpNode)
		{
			// Empty
		}


		template <typename T, typename Pointer, typename Reference>
		inline typename intrusive_list_iterator<T, Pointer, Reference>::reference
		intrusive_list_iterator<T, Pointer, Reference>::operator*() const
		{
			return *mpNode;
		}


		template <typename T, typename Pointer, typename Reference>
		inline typename intrusive_list_iterator<T, Pointer, Reference>::pointer
		intrusive_list_iterator<T, Pointer, Reference>::operator->() const
		{
			return mpNode;
		}


		template <typename T, typename Pointer, typename Reference>
		inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type&
		intrusive_list_iterator<T, Pointer, Reference>::operator++()
		{
			mpNode = static_cast<node_type*>(mpNode->mpNext);
			return *this;
		}


		template <typename T, typename Pointer, typename Reference>
		inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type
		intrusive_list_iterator<T, Pointer, Reference>::operator++(int)
		{
			intrusive_list_iterator it(*this);
			mpNode = static_cast<node_type*>(mpNode->mpNext);
			return it;
		}


		template <typename T, typename Pointer, typename Reference>
		inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type&
		intrusive_list_iterator<T, Pointer, Reference>::operator--()
		{
			mpNode = static_cast<node_type*>(mpNode->mpPrev);
			return *this;
		}


		template <typename T, typename Pointer, typename Reference>
		inline typename intrusive_list_iterator<T, Pointer, Reference>::this_type
		intrusive_list_iterator<T, Pointer, Reference>::operator--(int)
		{
			intrusive_list_iterator it(*this);
			mpNode = static_cast<node_type*>(mpNode->mpPrev);
			return it;
		}


		// The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
		// Thus we provide additional template paremeters here to support this. The defect report does not
		// require us to support comparisons between reverse_iterators and const_reverse_iterators.
		template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
		inline bool operator==(const intrusive_list_iterator<T, PointerA, ReferenceA>& a, 
							   const intrusive_list_iterator<T, PointerB, ReferenceB>& b)
		{
			return a.mpNode == b.mpNode;
		}


		template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
		inline bool operator!=(const intrusive_list_iterator<T, PointerA, ReferenceA>& a, 
							   const intrusive_list_iterator<T, PointerB, ReferenceB>& b)
		{
			return a.mpNode != b.mpNode;
		}


		// We provide a version of operator!= for the case where the iterators are of the 
		// same type. This helps prevent ambiguity errors in the presence of rel_ops.
		template <typename T, typename Pointer, typename Reference>
		inline bool operator!=(const intrusive_list_iterator<T, Pointer, Reference>& a, 
							   const intrusive_list_iterator<T, Pointer, Reference>& b)
		{
			return a.mpNode != b.mpNode;
		}




		///////////////////////////////////////////////////////////////////////
		// intrusive_list_base
		///////////////////////////////////////////////////////////////////////

		inline intrusive_list_base::intrusive_list_base() 
		{
			mAnchor.mpNext = mAnchor.mpPrev = &mAnchor;
		}


		inline bool intrusive_list_base::empty() const
		{
			return mAnchor.mpPrev == &mAnchor;
		}


		inline intrusive_list_base::size_type intrusive_list_base::size() const
		{
			const intrusive_list_node* p = &mAnchor;
			size_type n = (size_type)-1;

			do {
				++n;
				p = p->mpNext;
			} while(p != &mAnchor);

			return n;
		}


		inline void intrusive_list_base::clear()
		{
			mAnchor.mpNext = mAnchor.mpPrev = &mAnchor;
		}


		inline void intrusive_list_base::pop_front()
		{
			mAnchor.mpNext->mpNext->mpPrev = &mAnchor;
			mAnchor.mpNext = mAnchor.mpNext->mpNext;
		}


		inline void intrusive_list_base::pop_back()
		{
			mAnchor.mpPrev->mpPrev->mpNext = &mAnchor;
			mAnchor.mpPrev = mAnchor.mpPrev->mpPrev;
		}




		///////////////////////////////////////////////////////////////////////
		// intrusive_list
		///////////////////////////////////////////////////////////////////////

		template <typename T>
		inline intrusive_list<T>::intrusive_list()
		{
		}


		template <typename T>
		inline intrusive_list<T>::intrusive_list(const this_type& /*x*/)
		{
			// We intentionally ignore argument x.
		}


		template <typename T>
		inline typename intrusive_list<T>::this_type& intrusive_list<T>::operator=(const this_type& /*x*/)
		{ 
			return *this; // We intentionally ignore argument x.
		}


		template <typename T>
		inline typename intrusive_list<T>::iterator intrusive_list<T>::begin()
		{
			return iterator(static_cast<T*>(mAnchor.mpNext));
		}


		template <typename T>
		inline typename intrusive_list<T>::const_iterator intrusive_list<T>::begin() const
		{
			return const_iterator(static_cast<T*>(mAnchor.mpNext));
		}


		template <typename T>
		inline typename intrusive_list<T>::iterator intrusive_list<T>::end()
		{
			return iterator(static_cast<T*>(&mAnchor));
		}


		template <typename T>
		inline typename intrusive_list<T>::const_iterator intrusive_list<T>::end() const
		{
			return const_iterator(static_cast<const T*>(&mAnchor));
		}


		//template <typename T>
		//inline typename intrusive_list<T>::reverse_iterator intrusive_list<T>::rbegin()
		//{
		//    return reverse_iterator(iterator(static_cast<T*>(&mAnchor)));
		//}


		//template <typename T>
		//inline typename intrusive_list<T>::const_reverse_iterator intrusive_list<T>::rbegin() const
		//{
		//    return const_reverse_iterator(const_iterator(static_cast<const T*>(&mAnchor)));
		//}


		//template <typename T>
		//inline typename intrusive_list<T>::reverse_iterator intrusive_list<T>::rend()
		//{
		//    return reverse_iterator(iterator(static_cast<T*>(mAnchor.mpNext)));
		//}


		//template <typename T>
		//inline typename intrusive_list<T>::const_reverse_iterator intrusive_list<T>::rend() const
		//{
		//    return const_reverse_iterator(const_iterator(static_cast<const T*>(mAnchor.mpNext)));
		//}
		

		template <typename T>
		inline typename intrusive_list<T>::reference intrusive_list<T>::front()
		{
			return *static_cast<T*>(mAnchor.mpNext);
		}


		template <typename T>
		inline typename intrusive_list<T>::const_reference intrusive_list<T>::front() const
		{
			return *static_cast<const T*>(mAnchor.mpNext);
		}


		template <typename T>
		inline typename intrusive_list<T>::reference intrusive_list<T>::back()
		{
			return *static_cast<T*>(mAnchor.mpPrev);
		}


		template <typename T>
		inline typename intrusive_list<T>::const_reference intrusive_list<T>::back() const
		{
			return *static_cast<const T*>(mAnchor.mpPrev);
		}


		template <typename T>
		inline void intrusive_list<T>::push_front(T& x)
		{
			x.mpNext = mAnchor.mpNext;
			x.mpPrev = &mAnchor;
			mAnchor.mpNext = &x;
			x.mpNext->mpPrev = &x;
		}


		template <typename T>
		inline void intrusive_list<T>::push_back(T& x)
		{
			x.mpPrev = mAnchor.mpPrev;
			x.mpNext = &mAnchor;
			mAnchor.mpPrev = &x;
			x.mpPrev->mpNext = &x;
		}


		template <typename T>
		inline bool intrusive_list<T>::contains(const T& x) const
		{
			for(const intrusive_list_node* p = mAnchor.mpNext; p != &mAnchor; p = p->mpNext)
			{
				if(p == &x)
					return true;
			}

			return false;
		}


		template <typename T>
		inline typename intrusive_list<T>::iterator intrusive_list<T>::locate(T& x)
		{
			for(intrusive_list_node* p = (T*)mAnchor.mpNext; p != &mAnchor; p = p->mpNext)
			{
				if(p == &x)
					return iterator(static_cast<T*>(p));
			}

			return iterator((T*)&mAnchor);
		}


		template <typename T>
		inline typename intrusive_list<T>::const_iterator intrusive_list<T>::locate(const T& x) const
		{
			for(const intrusive_list_node* p = mAnchor.mpNext; p != &mAnchor; p = p->mpNext)
			{
				if(p == &x)
					return const_iterator(static_cast<const T*>(p));
			}

			return const_iterator((T*)&mAnchor);
		}


		template <typename T>
		inline typename intrusive_list<T>::iterator intrusive_list<T>::insert(iterator pos, T& x)
		{
			node_type& next = *pos;
			node_type& prev = *static_cast<node_type*>(next.mpPrev);
			prev.mpNext = next.mpPrev = &x;
			x.mpPrev    = &prev;
			x.mpNext    = &next;
			return iterator(&x);
		}


		template <typename T>
		inline typename intrusive_list<T>::iterator intrusive_list<T>::erase(iterator pos)
		{
			node_type& prev = *static_cast<node_type*>(pos.mpNode->mpPrev);
			node_type& next = *static_cast<node_type*>(pos.mpNode->mpNext);
			prev.mpNext = &next;
			next.mpPrev = &prev;
			return iterator(&next);
		}


		template <typename T>
		inline typename intrusive_list<T>::iterator intrusive_list<T>::erase(iterator pos, iterator last)
		{
			node_type& prev = *static_cast<node_type*>(pos.mpNode->mpPrev);
			node_type& next = *last.mpNode;
			prev.mpNext = &next;
			next.mpPrev = &prev;
			return last;
		}


		template <typename T>
		void intrusive_list<T>::swap(intrusive_list& x)
		{
			// swap anchors
			const intrusive_list_node temp(mAnchor);
			mAnchor   = x.mAnchor;
			x.mAnchor = temp;

			// Fixup node pointers into the anchor, since the addresses of 
			// the anchors must stay the same with each list.
			if(mAnchor.mpNext == &x.mAnchor)
				mAnchor.mpNext = mAnchor.mpPrev = &mAnchor;
			else
				mAnchor.mpNext->mpPrev = mAnchor.mpPrev->mpNext = &mAnchor;

			if(x.mAnchor.mpNext == &mAnchor)
				x.mAnchor.mpNext = x.mAnchor.mpPrev = &x.mAnchor;
			else
				x.mAnchor.mpNext->mpPrev = x.mAnchor.mpPrev->mpNext = &x.mAnchor;
		}


	} // namespace StdC

} // namespace EA




#endif // Header include guard


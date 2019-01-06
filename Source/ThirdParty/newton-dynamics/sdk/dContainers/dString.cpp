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
#include "dString.h"


#define D_USE_POOL_BUKECT_ALLOCATOR
#define D_STRING_MEM_GRANULARITY		16
#define D_STRING_MEM_MAX_BUCKET_SIZE	256
#define D_STRING_MEM_BUCKETS			(D_STRING_MEM_MAX_BUCKET_SIZE / D_STRING_MEM_GRANULARITY)
#define D_DSTRING_ENTRIES_IN_FREELIST	32




class dString::dStringAllocator
{
	public:
	#ifdef D_USE_POOL_BUKECT_ALLOCATOR
		class dMemBucket
		{
			public:

			class dDataChunk
			{
				public: 
				int m_size;
				int m_count;
				dDataChunk* m_next;
				
			};

			dMemBucket()
				:m_freeListDataChunk(NULL)
			{
			}
			~dMemBucket()
			{
			}

			void Prefetch (int chunckSize)
			{
				for (int i = 0; i < D_DSTRING_ENTRIES_IN_FREELIST; i ++) {
					//dDataChunk* const data = (dDataChunk*) new char[chunckSize + sizeof (int)];
					dDataChunk* const data = (dDataChunk*) dContainersAlloc::Alloc (chunckSize + sizeof (int));
					data->m_count = i + 1; 
					data->m_size = chunckSize;
					data->m_next = m_freeListDataChunk; 
					m_freeListDataChunk = data;
				}
			}

			void Flush ()
			{
				for (int i = 0; m_freeListDataChunk && (i < D_DSTRING_ENTRIES_IN_FREELIST); i ++) {
					dDataChunk* const ptr = m_freeListDataChunk;
					m_freeListDataChunk = m_freeListDataChunk->m_next;
					//delete[] (char*) ptr;
					dContainersAlloc::Free (ptr);
				}
			}

			char* Alloc(int size)
			{
				dAssert (size < 1024 * 4);
				if (!m_freeListDataChunk) {
					Prefetch (size);
				}
				dDataChunk* const data = m_freeListDataChunk;
				dAssert (size == data->m_size);
				m_freeListDataChunk = m_freeListDataChunk->m_next;
				return ((char*)data) + sizeof (int);
			}

			void Free(char * const ptr)
			{
				char* const realPtr = ptr - sizeof (int);
				dMemBucket::dDataChunk* const dataChunck = (dMemBucket::dDataChunk*) (realPtr);

				dataChunck->m_count = m_freeListDataChunk ? m_freeListDataChunk->m_count + 1 : 1;
				dataChunck->m_next = m_freeListDataChunk;
				m_freeListDataChunk = dataChunck;
				if (dataChunck->m_count >= 2 * D_DSTRING_ENTRIES_IN_FREELIST) {
					Flush();
				}
			}

			dDataChunk* m_freeListDataChunk;
		};

	
		dStringAllocator()
		{
			for (int i = 0; i < int (sizeof (m_buckects) / sizeof (m_buckects[0])); i ++) {
				m_buckects[i].Prefetch ((i + 1)* D_STRING_MEM_GRANULARITY);
			}
		}
		~dStringAllocator()
		{
			for (int i = 0; i < int (sizeof (m_buckects) / sizeof (m_buckects[0])); i ++) {
				m_buckects[i].Flush();
			}
		}

		char* Alloc(int size)
		{
			dAssert (size >= 1);
			if (size <= D_STRING_MEM_MAX_BUCKET_SIZE) {
				int buckectEntry = (size - 1) / D_STRING_MEM_GRANULARITY;
				int buckectSize = (buckectEntry + 1) * D_STRING_MEM_GRANULARITY;
				return m_buckects[buckectEntry].Alloc(buckectSize);
			}
			dMemBucket::dDataChunk* const ptr = (dMemBucket::dDataChunk*) dContainersAlloc::Alloc (size + sizeof (int));
			ptr->m_size = size;
			return ((char*)ptr) + sizeof (int);
		}

		void Free(char* const ptr)
		{
			char* const realPtr = ptr-sizeof (int);
			dMemBucket::dDataChunk* const dataChunck = (dMemBucket::dDataChunk*) (realPtr);
			if (dataChunck->m_size <= D_STRING_MEM_MAX_BUCKET_SIZE) {
				int buckectEntry = dataChunck->m_size / D_STRING_MEM_GRANULARITY - 1;
				m_buckects[buckectEntry].Free(ptr);
			} else {
				void* const ptr1 = ((char*)ptr) - sizeof (int);
				dContainersAlloc::Free (ptr1);
			}
		}

		dMemBucket m_buckects [D_STRING_MEM_BUCKETS];

	#else 
		char* Alloc(int size)
		{
			//return new char[size];
			return (char*) dContainersAlloc::Alloc (size);
		}

		void Free(char* const ptr)
		{
			//delete[] ptr;
			dContainersAlloc::Free (ptr);
		}
	#endif
};

//dString::dStringAllocator dString::m_allocator;

dString::dString ()
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
}

dString::dString (const dString& src)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	if (src.m_string) {
		m_size = src.m_size;
		m_capacity = m_size + 1;

		m_string = AllocMem (src.m_size + 1);
		CopyData (m_string, src.m_string, src.m_size + 1);
		m_string[m_size] = 0;
	}
}

dString::dString (const char* const data)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	if (data) {
		m_size = CalculateSize (data);
		m_capacity = m_size + 1; 

		m_string = AllocMem (m_size + 1);
		CopyData (m_string, data, m_size + 1);
		m_string[m_size] = 0;
	}
}

dString::dString (const char* const data, int maxSize)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	if (data) {
		m_size = dMin (CalculateSize (data), maxSize);
		m_capacity = m_size + 1; 
		m_string = AllocMem (m_size + 1);
		CopyData (m_string, data, m_size + 1);
		m_string[m_size] = 0;
	}
}

dString::dString (const dString& src, const char* const concatenate, int concatenateSize)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	m_string = AllocMem (src.m_size + concatenateSize + 1);
	memcpy (m_string, src.m_string, src.m_size);
	memcpy (&m_string[src.m_size], concatenate, concatenateSize);
	m_size = src.m_size + concatenateSize;
	m_string[m_size] = 0;
	m_capacity = m_size + 1;
}



dString::dString (char chr)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	m_string = AllocMem (2);
	m_string[0] = chr;
	m_string[1] = 0;
	m_size = 1;
	m_capacity = m_size + 1;
}

dString::dString (int val)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	char tmp[256];

	int count = 0;
	unsigned mag = abs (val);
	do {
		unsigned digit = mag % 10;
		mag /= 10;  
		tmp[count] = '0' + char(digit);
		count ++;
	} while (mag > 0);

	int offset = (val >= 0) ? 0: 1;
	m_string = AllocMem (count + offset + 1);
	if (offset) {
		m_string[0] = '-';
	}
	for (int i = 0; i < count; i ++) {
		m_string[i + offset] = tmp[count - i - 1];
	}

	m_string[count + offset] = 0;
	m_size = count + offset;
	m_capacity = m_size + 1;
}

dString::dString (long long val)
	:m_string(NULL)
	,m_size(0)
	,m_capacity(0)
{
	char tmp[256];

	int count = 0;
	unsigned long long mag = (val > 0ll) ? val : -val;
	do {
		unsigned long long digit = mag % 10ll;
		mag /= 10ll;  
		tmp[count] = '0' + char(digit);
		count ++;
	} while (mag > 0);

	int offset = (val >= 0ll) ? 0: 1;
	m_string = AllocMem (count + offset + 1);
	if (offset) {
		m_string[0] = '-';
	}
	for (int i = 0; i < count; i ++) {
		m_string[i + offset] = tmp[count - i - 1];
	}

	m_string[count + offset] = 0;
	m_size = count + offset;
	m_capacity = m_size + 1;
}


dString::~dString ()
{
	Empty();
}

void dString::Empty()
{
	if (m_capacity && m_string) {
		FreeMem (m_string);
	}
	m_size = 0;
	m_capacity = 0;
	m_string = NULL;
}

void dString::LoadFile (FILE* const file)
{
	Empty();
//	fseek (file, 0, SEEK_END);
//	int size = ftell (file);
	int size = 0;
	fseek (file, 0, SEEK_SET);
	for (;!feof(file); size ++) {
		fgetc (file);
	}
	fseek (file, 0, SEEK_SET);
	Expand (size);
	size_t ret = fread (m_string, 1, size, file);
	ret = 0;
	m_string[size-1] = 0;
	m_size = size-1;
	m_capacity = m_size + 1;
}



void dString::operator+= (const char* const src)
{
	char* const oldData = m_string;
	int size = CalculateSize (src);
	m_string = AllocMem (m_size + size + 1);
	memcpy (m_string, oldData, m_size);
	memcpy (&m_string[m_size], src, size);
	m_size = m_size + size;
	m_string[m_size] = 0;
	m_capacity = m_size + 1;
	FreeMem(oldData);
}


int dString::ToInteger() const
{
	int value = 0;
	if (m_size) {
		int base = (m_string[0] == '-') ? 1 : 0;
		for (int i = base; i < m_size; i ++) {
			char ch = m_string[i]; 		
			if ((ch >= '0') && (ch <= '9')) {
				value = value * 10 + ch - '0';
			} else {
				break;
			}
		}
		value *= base ? -1 : 1;
	}
	return value;
}


long long dString::ToInteger64() const
{
	long long value = 0;
	if (m_size) {
		int base = (m_string[0] == '-') ? 1 : 0;
		for (int i = base; i < m_size; i ++) {
			char ch = m_string[i]; 		
			if ((ch >= '0') && (ch <= '9')) {
				value = value * 10ll + ch - '0';
			} else {
				break;
			}
		}
		value *= base ? -1 : 1;
	}
	return value;
}


double dString::ToFloat() const
{
	double value = 0.0;
	double power = 1.0;
	double decimalBase = 1.0;
	if (m_size) {
		int base = (m_string[0] == '-') ? 1 : 0;
		for (int i = base; i < m_size; i ++) {
			char ch = m_string[i]; 		
			if ((ch >= '0') && (ch <= '9')) {
				value = value * 10ll + ch - '0';
				power *= decimalBase;
			} else if (ch == '.') {
				decimalBase = 10.0;
			} else {
				break;
			}
		}
		value *= base ? -1 : 1;
	}
	value /= power;

	return value;
}


dString& dString::operator= (const dString& src)
{
	if (m_capacity && m_string) {
		FreeMem (m_string);
	}
	m_string = NULL;
	m_capacity = 0;
	m_size = src.m_size;
	if (src.m_string) {
		m_capacity = src.m_size + 1;
		m_string = AllocMem (src.m_size + 1);
		CopyData (m_string, src.m_string, src.m_size + 1);
	}
	return *this;
}


int dString::CalculateSize (const char* const data) const
{
	int size = 0;
	if (data) {
		for (int i = 0; data[i]; i ++) {
			size ++;
		}
	}
	return size;
}

void dString::ToUpper()
{
	if (m_string) {
		for (char * cp = m_string; *cp; ++cp)
		{
			if ((*cp >= 'a') && (*cp <= 'z') )
				*cp += 'A' - 'a';
		}
	}
}


void dString::ToLower()
{
	if (m_string) {
		for (char * cp = m_string; *cp; ++cp)
		{
			if ((*cp >= 'A') && (*cp <= 'Z') )
				*cp += 'a' - 'A';
		}
	}
}

int dString::Find (char ch, int from) const
{
	for (int i = from; i < m_size; i ++) {
		if (m_string[i] == ch) {
			return i;
		}
	}
	return -1;
}

//int dString::Find (const dString& subStream, int from) const
int dString::Find (const char* const subString, int subStringLength, int from, int lenght) const
{
	dAssert (from >= 0);
	//dAssert (subStream.m_size >= 0);
	dAssert (subStringLength >= 1);

	int location = -1;
	if (m_size) {
		const int str2Size = dMin (subStringLength, lenght);
		if (str2Size == 1) {
			char ch = subString[0];
			const char* const ptr1 = m_string;
			for (int i = 0; i < m_size; i ++) {
				if (ch == ptr1[i]) {
					return i;
				}
			}
		} else if ((str2Size < 4) || (m_size < 64)) {
			const int size = m_size - str2Size;
			for (int j = from; j <= size; j ++) {
				const char* const ptr1 = &m_string[j];
				int i = 0;
				while (subString[i] && (ptr1[i] == subString[i])) {
					i ++;
				}
				if (!subString[i]) {
					return j;
				}
			}
		} else {
			// for large strings smart search
			short frequency[256];
			memset (frequency, -1, sizeof (frequency));
			for (int i = 0; i < str2Size; i ++) {
				frequency[int (subString[i])] = short(i);
			}

			int j = from;
			const int size = m_size - str2Size;
			while (j <= size) {
				const char* const ptr1 = &m_string[j];
				int i = str2Size - 1;
				while ((i >= 0) && (ptr1[i] == subString[i])) {
					i --;
				}
				if (i < 0) {
					return j;
					
				}
				j += dMax(i - frequency[int (ptr1[i])], 1);
			}
		}
	}
	return location;
}


void dString::Replace (int start, int size, const char* const str, int strSize)
{
	char* const oldData = m_string;
	m_string = AllocMem (m_size - size + strSize + 1);
	memcpy (m_string, oldData, start);
	memcpy (&m_string[start], str, strSize);
	memcpy (&m_string[start + strSize], &oldData[start + size], m_size - (start + size));
	m_size = m_size - size + strSize;
	m_capacity = m_size - size + strSize + 1;
	m_string[m_size] = 0;
	FreeMem(oldData);
}


void dString::Expand (int size)
{
	char* const oldData = m_string;
	m_string = AllocMem (m_size + size + 1);
	
	if (m_capacity) {
		memcpy (m_string, oldData, m_size);
		FreeMem(oldData);
	}
	m_string[m_size] = 0;
	m_capacity = m_size + size + 1;
}


dString::dStringAllocator& dString::GetAllocator() const
{
	static dStringAllocator allocator;
	return allocator;
}


char* dString::AllocMem(int size)
{
	return GetAllocator().Alloc(size);
}

void dString::FreeMem (char* const ptr)
{
	if (ptr) {
		GetAllocator().Free(ptr);
	}
}

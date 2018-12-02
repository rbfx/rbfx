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


#ifndef __dChatertSetMap_h_
#define __dChatertSetMap_h_



class dChatertSetMap 
{
	public:
	class ChatertSet
	{
		public:
		ChatertSet (const char* const set, int count, int id);
		bool IsCharAMatch (int id) const;
		int GetLength () const; 
		const char* GetSet () const;
		int GetId() const;

		private:
		static int sort (const void* a, const void* b);

		int m_id;
		int m_count;
		char m_characters[256];
		friend dChatertSetMap;
	};


	dChatertSetMap ();
	~dChatertSetMap();
	const ChatertSet* FindSet (int id) const;
	int AddSet (const char* const set, int count);

	const dTree<dList <dChatertSetMap::ChatertSet>::dListNode*, int>& GetSets() const;

	private:
	int m_id;
	dList <ChatertSet> m_sets;
	dTree<dList <ChatertSet>::dListNode*, int> m_crcID;
	dTree<dList <ChatertSet>::dListNode*, int> m_lable;
};




#endif
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


#ifndef __dAutomataState_h_
#define __dAutomataState_h_

#if (_MSC_VER >= 1400)
#	pragma warning (disable: 4201) //  warning C4201: nonstandard extension used : nameless struct/union
#endif



class dAutomataState 
{	
	public:
	enum TransitionType
	{
		EMPTY = 0,
		CHARACTER,
		CHARACTER_SET,
	};

	class dCharacter 
	{
		public:
		dCharacter ();
		dCharacter (int symbol);
		dCharacter (int info, TransitionType type);

		union {
			int m_symbol;
			struct 
			{
				int m_info			: 16;
				int m_type			: 16;
			};
		};
	};

	class dTransition 
	{
		public:
		dTransition (dCharacter character, dAutomataState* const targeState);

		dCharacter GetCharater () const;
		dAutomataState* GetState() const;

		private:
		dCharacter m_character;
		dAutomataState* m_targetdAutomataState; 
	};

	dAutomataState (int id);
	virtual ~dAutomataState ();

	void dAutomataState::GetStateArray (dList<dAutomataState*>& statesList);

	int m_id;
	int m_mark;
	bool m_exitState;
	dList<dTransition> m_transtions;
	dList<dAutomataState*> m_myNFANullStates;
};


#endif
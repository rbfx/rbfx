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

#ifndef _UNDO_REDO_H_
#define _UNDO_REDO_H_


class dUndoRedo
{
	public:
	dUndoRedo();
	virtual ~dUndoRedo();

	virtual void RestoreState() = 0;
	virtual dUndoRedo* CreateRedoState() const = 0;

};

class dUndoRedoManager 
{
	public:
	class dStack: public dList<dUndoRedo*>
	{
		public:
		dStack ();
		~dStack ();
		void Clear();

		void Push(dUndoRedo* const state);
		dUndoRedo* Pop();
	};

	dUndoRedoManager();
	~dUndoRedoManager();
	
	void Clear();
	void Push(dUndoRedo* const state);

	void Undo();
	void Redo();

	private:
	dStack m_undodStack;
	dStack m_redodStack;
};

#endif
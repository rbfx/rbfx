/////////////////////////////////////////////////////////////////////////////
// Name:        EditorScene.cpp
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
#include "dUndoRedo.h"


dUndoRedo::dUndoRedo() 
{
}


dUndoRedo::~dUndoRedo(void) 
{
}


dUndoRedoManager::dStack::dStack ()
	:dList<dUndoRedo*>()
{
}

dUndoRedoManager::dStack::~dStack ()
{
	Clear();
}

void dUndoRedoManager::dStack::Clear()
{
	for (dListNode* node = GetFirst(); node; node = node->GetNext()) {
		delete node->GetInfo();
	}
	RemoveAll();
}

void dUndoRedoManager::dStack::Push(dUndoRedo* const state)
{
	Append(state);
}

dUndoRedo* dUndoRedoManager::dStack::Pop()
{
	dUndoRedo* state = NULL;
	if (GetCount()) {
		state = GetLast()->GetInfo();
		Remove(GetLast());
	}
	return state;
}

dUndoRedoManager::dUndoRedoManager()
	:m_undodStack()
	,m_redodStack()
{
}

dUndoRedoManager::~dUndoRedoManager()
{
}


void dUndoRedoManager::Clear()
{
	m_undodStack.Clear();
	m_redodStack.Clear();
}

void dUndoRedoManager::Push(dUndoRedo* const state)
{
	m_redodStack.Clear();
	m_undodStack.Push(state);
}

void dUndoRedoManager::Redo()
{
	dUndoRedo* const state = m_redodStack.Pop();
	if (state) {
		m_undodStack.Push(state->CreateRedoState());
		state->RestoreState();
		delete state;
	}
}

void dUndoRedoManager::Undo()
{
	dUndoRedo* const state = m_undodStack.Pop();
	if (state) {
		m_redodStack.Push(state->CreateRedoState());
		state->RestoreState();
		delete state;
	}
}


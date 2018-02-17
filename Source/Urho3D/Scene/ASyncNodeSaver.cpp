#include "ASyncNodeSaver.h"

#include "../Scene/Scene.h"
#include "../Core/CoreEvents.h"
#include "../IO/File.h"

namespace Urho3D {


	ASyncNodeSaver::ASyncNodeSaver(Context* context) : Object(context)
	{
	}

	ASyncNodeSaver::~ASyncNodeSaver()
	{
	}

	void ASyncNodeSaver::RegisterObject(Context* context)
	{
		context->RegisterFactory<ASyncNodeSaver>();
	}

	void ASyncNodeSaver::StartSave(File* file, Node* node)
	{
		mFile = file;

		//start the saving process
		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ASyncNodeSaver, HandleUpdate));
		isSaving = true;
		mIsInError = false;
		mRootNode = node;

	}


	void ASyncNodeSaver::StartSave(String filePath, Node* node)
	{
		StartSave(new File(context_, filePath, FILE_WRITE), node);
	}

	void ASyncNodeSaver::CancelSaving()
	{
		endSave();
	}

	Node* ASyncNodeSaver::FinishedNode() const
	{
		return mRootNode;
	}

	bool ASyncNodeSaver::IsError()
	{
		return mIsInError;
	}

	void ASyncNodeSaver::continueSaving()
	{
		for (int i = 0; i < mNodesPerFrame && isSaving; i++)
			processNextNode();
	}


	void ASyncNodeSaver::processNextNode()
	{
		if (mLoadStack.Size())
		{
			SaveLevel& curLevel = mLoadStack.Back();
			if (curLevel.curChild <= (curLevel.childrenCount - 1)) {
				curLevel.curChild++;

				PushAndSave(curLevel.node->GetChildren()[curLevel.curChild - 1]);
			}
			else
			{
				mLoadStack.Pop();
				if (mLoadStack.Size() == 0)
					endSave();
			}
		}
		else
		{
			//if rootnode is already defined - we are in inplace mode.
			if (mRootNode != nullptr)
			{
				PushAndSave(mRootNode);
			}
			
		}
	}

	void ASyncNodeSaver::PushAndSave(Node* node)
	{
		SaveLevel newLevel;
		newLevel.node = node;
		newLevel.childrenCount = node->GetNumChildren();
		newLevel.curChild = 0;

		mLoadStack.Push(newLevel);

		node->Save(*mFile, false);
	}







	void ASyncNodeSaver::endSave() {
		isSaving = false;
		mRootNode = nullptr;
		mParentNode = nullptr;
		mFile = nullptr;
		UnsubscribeFromEvent(E_UPDATE);
	}

	void ASyncNodeSaver::HandleUpdate(StringHash eventName, VariantMap& eventData)
	{
		continueSaving();
	}


}
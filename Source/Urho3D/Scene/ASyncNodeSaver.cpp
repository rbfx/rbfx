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

	void ASyncNodeSaver::StartLoad(File* file, Node* node, bool inPlace)
	{
		mFile = file;
		startStreamPos = file->GetPosition();

		//start the loading process
		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ASyncNodeSaver, HandleUpdate));
		isLoading = true;
		mIsInError = false;
		mInPlaceRoot = inPlace;
		mSceneResolver.Reset();

		if (inPlace)
		{
			mParentNode = nullptr;
			mRootNode = node;
		}
		else
		{
			mParentNode = node;
			mRootNode = nullptr;
		}
	}


	void ASyncNodeSaver::StartLoad(String filePath, Node* node, bool inPlace /*= false*/)
	{
		StartLoad(new File(context_, filePath, FILE_READ), node, inPlace);
	}

	void ASyncNodeSaver::CancelLoading()
	{
		endLoad();
	}

	Node* ASyncNodeSaver::FinishedNode() const
	{
		return mRootNode;
	}

	bool ASyncNodeSaver::IsError()
	{
		return mIsInError;
	}

	void ASyncNodeSaver::continueLoading()
	{
		for (int i = 0; i < mNodesPerFrame && isLoading; i++)
			processNextNode();
	}


	void ASyncNodeSaver::processNextNode()
	{
		if (mLoadStack.Size())
		{
			LoadLevel& curLevel = mLoadStack.Back();
			if (curLevel.curChild <= (curLevel.childrenCount - 1)) {
				curLevel.curChild++;
				if (!createNodeAndPushToStack(curLevel.node))
					endLoad();
			}
			else
			{
				mLoadStack.Pop();
				if (mLoadStack.Size() == 0)
					endLoad();
			}
		}
		else
		{
			//if rootnode is already defined - we are in inplace mode.
			if (mRootNode != nullptr)
				LoadNodeAndPushToStack(mRootNode);
			else
				mRootNode = createNodeAndPushToStack(mParentNode);
		}

	}

	void ASyncNodeSaver::endLoad() {
		isLoading = false;
		mRootNode = nullptr;
		mParentNode = nullptr;
		mFile = nullptr;
		UnsubscribeFromEvent(E_UPDATE);
	}

	void ASyncNodeSaver::HandleUpdate(StringHash eventName, VariantMap& eventData)
	{
		continueLoading();
	}


	//creates a new node, loads it, and pushes the info to the stack.
	Node* ASyncNodeSaver::createNodeAndPushToStack(Node* parent)
	{
		Node* newNode = parent->CreateChild();
		//record this load to the stack
		LoadLevel loadState;
		loadState.node = newNode;
		loadState.nodeId = mFile->ReadUInt();
		loadState.curChild = 0;

		loadNodeAndChildInfo(newNode, loadState);


		mLoadStack.Push(loadState);

		return newNode;
	}

	Node* ASyncNodeSaver::LoadNodeAndPushToStack(Node* existingNode)
	{

		//record this load to the stack
		LoadLevel loadState;
		loadState.node = existingNode;
		loadState.nodeId = mFile->ReadUInt();
		loadState.curChild = 0;

		loadNodeAndChildInfo(existingNode, loadState);

		mLoadStack.Push(loadState);

		return existingNode;
	}


	bool ASyncNodeSaver::loadNodeAndChildInfo(Node* newNode, LoadLevel &loadState)
	{
		bool loadSuccess = newNode->Load(*mFile, mSceneResolver, false);
		if (!loadSuccess) {
			mIsInError = true;
			return false;
		}
		loadState.childrenCount = mFile->ReadVLE();
		return true;
	}
}
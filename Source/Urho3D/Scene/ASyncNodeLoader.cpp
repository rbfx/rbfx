#include "ASyncNodeLoader.h"
#include "../Scene/Scene.h"
#include "../Core/CoreEvents.h"
#include "../IO/File.h"

namespace Urho3D {


	ASyncNodeLoader::ASyncNodeLoader(Context* context) : Object(context)
	{
	}

	ASyncNodeLoader::~ASyncNodeLoader()
	{
	}

	void ASyncNodeLoader::StartLoad(File* file, Node* node, bool inPlace)
	{
		mFile = file;
		startStreamPos = file->GetPosition();

		//start the loading process
		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ASyncNodeLoader, HandleUpdate));
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


	void ASyncNodeLoader::StartLoad(String filePath, Node* node, bool inPlace /*= false*/)
	{
		StartLoad(new File(context_, filePath, FILE_READ), node, inPlace);
	}

	void ASyncNodeLoader::CancelLoading()
	{
		endLoad();
	}

	Node* ASyncNodeLoader::FinishedNode() const
	{
		return mRootNode;
	}

	bool ASyncNodeLoader::IsError()
	{
		return mIsInError;
	}

	void ASyncNodeLoader::continueLoading()
	{
		for(int i = 0; i < mNodesPerFrame && isLoading; i++)
			processNextNode();
	}


	void ASyncNodeLoader::processNextNode()
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

	void ASyncNodeLoader::endLoad() {
		isLoading = false;
		mRootNode = nullptr;
		mParentNode = nullptr;
		UnsubscribeFromEvent(E_UPDATE);
	}

	void ASyncNodeLoader::HandleUpdate(StringHash eventName, VariantMap& eventData)
	{
		continueLoading();
	}


	//creates a new node, loads it, and pushes the info to the stack.
	Node* ASyncNodeLoader::createNodeAndPushToStack(Node* parent)
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

	Node* ASyncNodeLoader::LoadNodeAndPushToStack(Node* existingNode)
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
	
	
	bool ASyncNodeLoader::loadNodeAndChildInfo(Node* newNode, LoadLevel &loadState)
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
#include "ASyncNodeUnloader.h"
#include "../Scene/Scene.h"
#include "../Core/CoreEvents.h"
#include "../IO/File.h"
namespace Urho3D
{

	ASyncNodeUnLoader::ASyncNodeUnLoader(Context* context) : Object(context)
	{

	}

	ASyncNodeUnLoader::~ASyncNodeUnLoader()
	{

	}






	void ASyncNodeUnLoader::StartUnLoad(Node* node)
	{
		mChildren.Clear();

		//get all the children in order of root to leafs.
		PODVector<Node *> children = node->GetChildren(true);

		//store in order of leafs to roots and form the whole tree.
		for (int i = children.Size() - 1; i >= 0; i--) {

			mChildren.Push(WeakPtr<Node>(children[i]));
		}

		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ASyncNodeUnLoader, HandleUpdate));
		isUnLoading = true;
		mRootNode = node;
	}

	void ASyncNodeUnLoader::CancelUnLoading()
	{
		endUnload();
	}

	void ASyncNodeUnLoader::continueUnLoading()
	{
		for (int i = 0; i < mNodesPerFrame && isUnLoading; i++)
			processNextNode();
	}

	void ASyncNodeUnLoader::processNextNode()
	{
		if (mChildren.Size()) {

			if (!mChildren.Back().Expired())
				mChildren.Back()->Remove();

			mChildren.Pop();

			if (mChildren.Size() == 0)
			{
				if (!mRootNode.Expired())
					mRootNode->Remove();
				endUnload();
			}
		}
	}

	void ASyncNodeUnLoader::endUnload()
	{
		isUnLoading = false;
		mRootNode = nullptr;
		UnsubscribeFromEvent(E_UPDATE);
	}

	void ASyncNodeUnLoader::HandleUpdate(StringHash eventName, VariantMap& eventData)
	{
		continueUnLoading();
	}

}
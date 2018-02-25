#include "ASyncNodeRemover.h"
#include "../Scene/Scene.h"
#include "../Core/CoreEvents.h"
#include "../IO/File.h"
namespace Urho3D
{

	ASyncNodeRemover::ASyncNodeRemover(Context* context) : Object(context)
	{

	}

	ASyncNodeRemover::~ASyncNodeRemover()
	{

	}






	void ASyncNodeRemover::StartRemove(Node* node, bool removeRoot)
	{
		mChildren.Clear();

		//get all the children in order of root to leafs.
		PODVector<Node *> children = node->GetChildren(true);

		//store in order of leafs to roots and form the whole tree.
		for (int i = children.Size() - 1; i >= 0; i--) {

			mChildren.Push(WeakPtr<Node>(children[i]));
		}

		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ASyncNodeRemover, HandleUpdate));
		isUnLoading = true;
		mRootNode = node;
		mRemoveRoot = removeRoot;
	}

	void ASyncNodeRemover::CancelRemove()
	{
		endRemove();
	}

	void ASyncNodeRemover::continueRemove()
	{
		for (int i = 0; i < mNodesPerFrame && isUnLoading; i++)
			processNextNode();
	}

	void ASyncNodeRemover::processNextNode()
	{
		if (mChildren.Size()) {

			if (!mChildren.Back().Expired())
				mChildren.Back()->Remove();

			mChildren.Pop();

			if (mChildren.Size() == 0)
			{
				if (!mRootNode.Expired())
					mRootNode->Remove();
				endRemove();
			}
		}
	}

	void ASyncNodeRemover::endRemove()
	{
		isUnLoading = false;
		mRootNode = nullptr;
		mRemoveRoot = true;
		UnsubscribeFromEvent(E_UPDATE);
	}

	void ASyncNodeRemover::HandleUpdate(StringHash eventName, VariantMap& eventData)
	{
		continueRemove();
	}

}
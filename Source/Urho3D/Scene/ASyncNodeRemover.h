#pragma once



#pragma once
#include "../Precompiled.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Scene/Node.h"

namespace Urho3D
{
	class URHO3D_API ASyncNodeRemover : public Object {

		URHO3D_OBJECT(ASyncNodeRemover, Object);

	public:
		explicit ASyncNodeRemover(Context* context);
		virtual ~ASyncNodeRemover();

		static void RegisterObject(Context* context)
		{
			context->RegisterFactory<ASyncNodeRemover>();
		}


		/// Starts removeing the given node by removing its children over time.  The given node is removed if removeRoot is true.
		void StartRemove(Node* node, bool removeRoot = true);

		/// set how many nodes to remove per frame
		void SetNodesPerFrame(int nodesPerFrame) { mNodesPerFrame = nodesPerFrame; }

		/// return how many nodes are loaded per frame.
		int GetNodesPerFrame() const { return mNodesPerFrame; }

		/// returns true if loading is in progress.
		bool IsRemoving() const { return isUnLoading; }

		/// Cancels the current loading process.
		void CancelRemove();


	protected:



		void continueRemove();
		void processNextNode();
		void endRemove();

		void HandleUpdate(StringHash eventName, VariantMap& eventData);

		Vector<WeakPtr<Node>> mChildren;

		bool isUnLoading = false;
		WeakPtr<Node> mRootNode;
		int mNodesPerFrame = 10;
		bool mRemoveRoot = true;
	};


}



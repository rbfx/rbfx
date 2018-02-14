#pragma once



#pragma once
#include "../Precompiled.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Scene/Node.h"

namespace Urho3D
{
	class URHO3D_API ASyncNodeUnLoader : public Object {

		URHO3D_OBJECT(ASyncNodeUnLoader, Object);

	public:
		explicit ASyncNodeUnLoader(Context* context);
		virtual ~ASyncNodeUnLoader();

		static void RegisterObject(Context* context)
		{
			context->RegisterFactory<ASyncNodeUnLoader>();
		}


		/// Starts removeing the given node by removing its children over time.
		void StartUnLoad(Node* node);

		/// set how many nodes to remove per frame
		void SetNodesPerFrame(int nodesPerFrame) { mNodesPerFrame = nodesPerFrame; }

		/// return how many nodes are loaded per frame.
		int GetNodesPerFrame() const { return mNodesPerFrame; }

		/// returns true if loading is in progress.
		bool IsUnLoading() const { return isUnLoading; }

		/// Cancels the current loading process.
		void CancelUnLoading();


	protected:



		void continueUnLoading();
		void processNextNode();
		void endUnload();

		void HandleUpdate(StringHash eventName, VariantMap& eventData);

		Vector<WeakPtr<Node>> mChildren;

		bool isUnLoading = false;
		WeakPtr<Node> mRootNode;
		int mNodesPerFrame = 10;
	};


}



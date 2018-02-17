#pragma once

#include "../Precompiled.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../Scene/Node.h"
#include "../Scene/SceneResolver.h"

namespace Urho3D
{
	class File;


	class URHO3D_API ASyncNodeSaver : public Object {

		URHO3D_OBJECT(ASyncNodeSaver, Object);

	public:
		explicit ASyncNodeSaver(Context* context);
		virtual ~ASyncNodeSaver();

		static void RegisterObject(Context* context)
		{
			context->RegisterFactory<ASyncNodeSaver>();
			//attributes here
		}

		struct LoadLevel {
			WeakPtr<Node> node;
			unsigned nodeId;
			int childrenCount = 0;
			int curChild = 0;
		};

		/// Starts Loading the node as a child of the given node or inplace on the given node.
		void StartLoad(File* file, Node* node, bool inPlace = false);
		void StartLoad(String filePath, Node* node, bool inPlace = false);

		/// set how many nodes to load per frame
		void SetNodesPerFrame(int nodesPerFrame) { mNodesPerFrame = nodesPerFrame; }

		/// return how many nodes are loaded per frame.
		int GetNodesPerFrame() const { return mNodesPerFrame; }

		/// returns true if loading is in progress.
		bool IsLoading() const { return isLoading; }

		/// Cancels the current loading process.
		void CancelLoading();

		/// returns the new node after loading has finished else returns nullptr;
		Node* FinishedNode() const;

		/// returns true if something went wrong in the loading process.
		bool IsError();

	protected:


		void continueLoading();
		void processNextNode();
		void endLoad();


		Node* createNodeAndPushToStack(Node* parent);
		Node* LoadNodeAndPushToStack(Node* existingNode);


		bool loadNodeAndChildInfo(Node* newNode, LoadLevel &loadState);


		void HandleUpdate(StringHash eventName, VariantMap& eventData);


		bool isLoading = false;
		unsigned startStreamPos = 0;
		SharedPtr<File> mFile;
		SceneResolver mSceneResolver;

		Vector<LoadLevel> mLoadStack;
		WeakPtr<Node> mParentNode;
		WeakPtr<Node> mRootNode;
		bool mInPlaceRoot = false;
		bool mIsInError = false;
		int mNodesPerFrame = 10;
	};


}



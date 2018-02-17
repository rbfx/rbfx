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

		static void RegisterObject(Context* context);

		struct SaveLevel {
			WeakPtr<Node> node;
			unsigned nodeId;
			int childrenCount = 0;
			int curChild = 0;
		};

		/// Starts Saving the node and its children.
		void StartSave(File* file, Node* node);
		void StartSave(String filePath, Node* node);

		/// set how many nodes to load per frame
		void SetNodesPerFrame(int nodesPerFrame) { mNodesPerFrame = nodesPerFrame; }

		/// return how many nodes are saved per frame.
		int GetNodesPerFrame() const { return mNodesPerFrame; }

		/// returns true if saving is in progress.
		bool IsSaving() const { return isSaving; }

		/// Cancels the current saving process.
		void CancelSaving();

		/// returns the new node after loading has finished else returns nullptr;
		Node* FinishedNode() const;

		/// returns true if something went wrong in the loading process.
		bool IsError();

	protected:


		void continueSaving();
		void processNextNode();
		void PushAndSave(Node* node);
		void endSave();




		void HandleUpdate(StringHash eventName, VariantMap& eventData);


		bool isSaving = false;
		SharedPtr<File> mFile;
		Vector<SaveLevel> mLoadStack;
		WeakPtr<Node> mParentNode;
		WeakPtr<Node> mRootNode;
		bool mIsInError = false;
		int mNodesPerFrame = 10;
	};


}



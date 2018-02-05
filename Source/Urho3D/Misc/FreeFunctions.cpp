#include "../Misc/FreeFunctions.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Graphics/OctreeQuery.h"
#include "../Scene/Node.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"

namespace Urho3D {

	FreeFunctions::FreeFunctions(Context* context_) : Object(context_)
	{

	}

	void FreeFunctions::RegisterObject(Context* context)
	{
		context->RegisterSubsystem(new FreeFunctions(context));
	}



	bool FreeFunctions::SaveStringToFile(String& str, String fileFullPath)
	{
		SharedPtr<File> file = SharedPtr<File>(new File(context_, fileFullPath, Urho3D::FileMode::FILE_WRITE));
		unsigned int bytesWritten = file->Write(str.CString(), str.Length());
		if (bytesWritten != str.Length())
			return false;
		return true;
	}

	bool FreeFunctions::ReadFileToString(String& str, String fileFullPath)
	{
		SharedPtr<File> inFile = SharedPtr<File>(new File(context_, fileFullPath, Urho3D::FileMode::FILE_READ));
		unsigned int fileSize = inFile->GetSize();
		str.Resize(fileSize);
		unsigned int bytesRead = inFile->Read((char*)str.CString(), fileSize);
		return (bytesRead == fileSize);
	}

	String FreeFunctions::GetResourceBinDir() {
		if (GetSubsystem<ResourceCache>()->GetResourceDirs().Size())
			return GetParentPath(GetSubsystem<ResourceCache>()->GetResourceDirs().Front());
		else
			return "";
	}

	String FreeFunctions::GetResourceDataDir()
	{
		if (!GetResourceBinDir().Empty())
			return GetResourceBinDir() + "Data";
		else
			return "";
	}

	String FreeFunctions::GetResourceCoreDataDir()
	{
		if (!GetResourceBinDir().Empty())
			return GetResourceBinDir() + "CoreData";
		else
			return "";
	}




	void PrintRayQueryResults(PODVector<RayQueryResult>& results)
	{
		URHO3D_LOGINFO("Printing RayQueryResults:");
		int i = 0;
		for (auto r : results) {
			URHO3D_LOGINFO("RayQuery: " + String(i));
			URHO3D_LOGINFO("distance_: " + String(r.distance_));
			URHO3D_LOGINFO("drawable_: " + String((long long)(void*)r.drawable_));
			URHO3D_LOGINFO("node_: " + String((long long)(void*)r.node_));
			URHO3D_LOGINFO("node_ name: " + String(r.node_->GetName()));
			URHO3D_LOGINFO("components: ");
			auto components = r.node_->GetComponents();
			for (auto c : components) {
				URHO3D_LOGINFO("	" + String(c->GetTypeName()));
			}

			URHO3D_LOGINFO("normal_: " + String(r.normal_));
			URHO3D_LOGINFO("position_: " + String(r.position_));
			URHO3D_LOGINFO("subObject_: " + String(r.subObject_));
			URHO3D_LOGINFO("textureUV_: " + String(r.textureUV_));
			URHO3D_LOGINFO("");
			i++;
		}
	}
}
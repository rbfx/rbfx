#include "../Misc/FreeFunctions.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Graphics/OctreeQuery.h"
#include "../Scene/Node.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"

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

	bool FreeFunctions::TestFileIsAscii(String fileFullPath)
	{
		SharedPtr<File> inFile = SharedPtr<File>(new File(context_, fileFullPath, Urho3D::FileMode::FILE_READ));
		int byte = inFile->ReadByte();
		while (byte > 0 && !inFile->IsEof()) {
			byte = inFile->ReadByte();
		}
		if (!inFile->IsEof())
			return false;
		
		return true;
	}

	bool FreeFunctions::TestFileIsUrhoNode(String fileFullPath)
	{
		SharedPtr<File> inFile = SharedPtr<File>(new File(context_, fileFullPath, Urho3D::FileMode::FILE_READ));
		SharedPtr<Node> node = context_->CreateObject<Node>();
		bool binLoad = node->Load(*inFile);
		if (binLoad)
			return true;

		SharedPtr<XMLFile> xmlFile = context_->CreateObject<XMLFile>();
		inFile->Seek(0);
		String textContent = inFile->ReadText();
		xmlFile->FromString(textContent);
		String rootElementName = xmlFile->GetRoot().GetName();
		if (rootElementName.ToLower() == "node" || rootElementName.ToLower() == "scene")
			return true;


		//#todo test json


		return false;
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

	void URHO3D_API PrintVariantMap(VariantMap& map)
	{
		URHO3D_LOGINFO("Printing Variant Map Of Size " + String(map.Size()));
		for (auto pair : map) {
			URHO3D_LOGINFO("Key{" + String(pair.first_) + "} value{" + String(pair.second_) + "}");
		}
	}

	String URHO3D_API AttributeInfoString(Serializable& serializable)
	{
		const Vector<AttributeInfo>* attributes = serializable.GetAttributes();
		if (!attributes)
			return "";

		Variant value;
		JSONValue attributesValue;
		String str;
		for (unsigned i = 0; i < attributes->Size(); ++i)
		{
			const AttributeInfo& attr = attributes->At(i);

			str += attr.name_ + " | " + serializable.GetAttribute(i).ToString() + "\n";
		}
		return str;
	}

}

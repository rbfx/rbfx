
#pragma once

#include "../Core/Object.h"
#include "../Container/Str.h"
#include "../IO/File.h"
#include "../IO/Log.h"
#include "../Graphics/OctreeQuery.h"
#include "../Core/Context.h"
#include "../Scene/Node.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../nativefiledialog/src/include/nfd.h"

namespace Urho3D {
	/// holds generic static functions that belong in the global namespace.  This class can be thought of as a temprorary holding space for generic functions that need a context.
	class URHO3D_API FreeFunctions : public Object {

		URHO3D_OBJECT(FreeFunctions, Object);

	public:

		FreeFunctions(Context* context_);
		static void RegisterObject(Context* context);

		bool SaveStringToFile(String& str, String fileFullPath);
		bool ReadFileToString(String& str, String fileFullPath);


		bool TestFileIsAscii(String fileFullPath);
		bool TestFileIsUrhoNode(String fileFullPath);

	};

	/// Other Free Functions that could be moved to elsewhere within the Urho3D namespace!


	void URHO3D_API PrintRayQueryResults(PODVector<RayQueryResult>& results);
	void URHO3D_API PrintVariantMap(VariantMap& map);

	//print all attributes from serializable into a string.
	String URHO3D_API AttributeInfoString(Serializable& serializable);



}


namespace Urho3D {

	String URHO3D_API GetNativeDialogSave(String startDirectory, const String& fileFilter);

	String URHO3D_API GetNativeDialogExistingDir(String startDirectory);

	String URHO3D_API GetNativeDialogExistingFile(String startDirectory, const String& fileFilter);
}

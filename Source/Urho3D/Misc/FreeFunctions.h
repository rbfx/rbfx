
#pragma once

#include "../Core/Object.h"
#include "../Container/Str.h"
#include "../IO/File.h"
#include "../Graphics/OctreeQuery.h"

namespace Urho3D {
	/// holds generic static functions that belong in the global namespace.  This class can be thought of as a temprorary holding space for generic functions that need a context.
	class URHO3D_API FreeFunctions : public Object {

		URHO3D_OBJECT(FreeFunctions, Object);

	public:

		FreeFunctions(Context* context_);
		static void RegisterObject(Context* context);

		bool SaveStringToFile(String& str, String fileFullPath);

		bool ReadFileToString(String& str, String fileFullPath);

		String GetResourceBinDir();
		String GetResourceDataDir();
		String GetResourceCoreDataDir();
	};

	/// returns how many times the character ch occurs in str.
	unsigned int StringCount(const String& str, char ch);


	void PrintRayQueryResults(PODVector<RayQueryResult>& results);
}
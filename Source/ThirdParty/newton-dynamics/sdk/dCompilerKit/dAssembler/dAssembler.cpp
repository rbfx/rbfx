// dTestAssembler.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <tchar.h>
#include <dVirtualMachine.h>
#include <dAssemblerCompiler.h>

#ifdef _MSC_VER
#include <windows.h>
#include <crtdbg.h>
#endif


int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _MSC_VER
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	if (argc < 2) {
		fprintf (stdout, "usage: dAssembly filename\n");
		exit (0);
	}



	const char* const sourceFileName = argv[1];
	FILE* const sourceFile = fopen (sourceFileName, "r");
	if (!sourceFile) {
		fprintf (stdout, "Rule file \"%s\" not found\n", sourceFile);
		exit (0);
	}

	fseek (sourceFile, 0, SEEK_END);
	int size = ftell (sourceFile);
	fseek (sourceFile, 0, SEEK_SET);

	char* const source = new char [size + 1];
	memset (source, 0, size + 1);
	fread (source, 1, size, sourceFile);
	fclose (sourceFile);

	
	dAssemblerCompiler compiler;
	dVirtualMachine virtualMachine;

	compiler.CompileSource (&virtualMachine, source);

	delete source;
	return 0;
}


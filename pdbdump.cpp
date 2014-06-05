/************************************************************************/
/* Copyright 2002 Andrew de Quincey                                     */
/* All Rights Reserved                                                  */
/*                                                                      */
/* Released under a BSD-style license. See file LICENSE.txt for info.   */
/*                                                                      */
/* Implementation of main entry point                                   */
/************************************************************************/

#include <dia2.h>
//#include <dialib.h>
#include "utils.h"
#include "PdbRender.h"


/// <summary>Entry point</summary>
///
int main(int argc, char* argv[]) {
  // Process args
  if (argc != 2) {
    fprintf(stderr, "Syntax: pdbdump <PDB filename>\n");
    return(1);
  }

  // Convert filename
  wchar_t pdbFilename[256];
  mbstowcs(pdbFilename, argv[1], 256);

  try {
    // open the pdb file
    CComPtr<IDiaDataSource> diaDataSource;
    CComPtr<IDiaSession> diaSession;
    CComPtr<IDiaSymbol> globalScope;
    openPdbFile(pdbFilename, &diaDataSource, &diaSession, &globalScope);

    // dump typedefs
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Typedefs\n\n");
    renderTypedefs(stdout, globalScope);

    // dump enumerations at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Enumerations\n\n");
    renderEnumerations(stdout, globalScope);

    // dump constants at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Constants\n\n");
    renderConstants(stdout, globalScope);

    // dump structs at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Structs\n\n");
    renderStructs(stdout, globalScope);

    // dump classes at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Classes\n\n");
    renderClasses(stdout, globalScope);

    // dump unions at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Unions\n\n");
    renderUnions(stdout, globalScope);

    // dump functions at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Functions\n\n");
    renderFunctions(stdout, globalScope);

    // dump globals and statics at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Globals/Statics\n\n");
    renderStaticAndGlobalData(stdout, globalScope);

    // dump public symbols at the global scope
    wprintf(L"\n//////////////////////////////////////////////////////////////////////////\n");
    wprintf(L"Public symbols\n\n");
    renderPublicSymbols(stdout, globalScope);

  } catch (runtime_error& e) {
    // deal with exception
    fprintf(stderr, "ERROR: ");
    fprintf(stderr, e.what());
    fprintf(stderr, "\n");
    exit(1);
  }

  // All was OK
  return(0);
}


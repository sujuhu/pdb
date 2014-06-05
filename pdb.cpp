#include <stdlib.h>
#include <wchar.h>
#include <dia2.h>
#include "PrintSymbol.h"
#include "callback.h"
#include <map.h>
#define INVALID_PDB			(int)0

typedef struct _pdb_t {
	IDiaDataSource *g_pDiaDataSource;
	IDiaSession *g_pDiaSession;
	IDiaSymbol *g_pGlobalSymbol;
	map_t 	modules;
  uint32_t g_dwMachineType = CV_CFL_80386;
}pdb_t;



////////////////////////////////////////////////////////////
// Retreive the table that matches the given iid
//
//  A PDB table could store the section contributions, the frame data,
//  the injected sources
//
HRESULT GetTable(IDiaSession *pSession, REFIID iid, void **ppUnk)
{
  IDiaEnumTables *pEnumTables;

  if (FAILED(pSession->getEnumTables(&pEnumTables))) {
    wprintf(L"ERROR - GetTable() getEnumTables\n");

    return E_FAIL;
  }

  IDiaTable *pTable;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumTables->Next(1, &pTable, &celt)) && (celt == 1)) {
    // There's only one table that matches the given IID

    if (SUCCEEDED(pTable->QueryInterface(iid, (void **) ppUnk))) {
      pTable->Release();
      pEnumTables->Release();

      return S_OK;
    }

    pTable->Release();
  }

  pEnumTables->Release();

  return E_FAIL;
}

typedef struct _pdb_module{
	wchar_t name[256];
}pdb_module_t;

////////////////////////////////////////////////////////////
// Dump all the modules information
//
bool dump_modules(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
	wprintf(L"\n\n** *MODULES\n\n");

  // Retrieve all the compiland symbols

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
    return false;
  }

  IDiaSymbol *pCompiland;
  ULONG celt = 0;
  ULONG iMod = 1;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
    BSTR bstrName;

    if (pCompiland->get_name(&bstrName) != S_OK) {
      wprintf(L"ERROR - Failed to get the compiland's name\n");

      pCompiland->Release();
      pEnumSymbols->Release();

      return false;
    }

	wprintf(L"%04X %s\n", iMod++, bstrName);
    map_node_t* module = malloc(sizeof(map_node_t));
    if (module == NULL) {
    	return false;
    }
    memset(module, 0, sizeof(map_node_t));

   	wcsncpy(module->name, bstrName, wcslen(bstrName));
    map_insert(&pdb->modules, ,module);

    // Deallocate the string allocated previously by the call to get_name

    SysFreeString(bstrName);

    pCompiland->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}

#define NAME_SIZE  256
typedef struct _pdb_public
{
  uint32_t  rva;
  uint32_t  seg;
  uint32_t  raw;
  uint32_t  sym_tag;
  wchar_t   name[NAME_SIZE];
}pdb_public_t;

////////////////////////////////////////////////////////////
// Dump all the public symbols - SymTagPublicSymbol
//
bool dump_publics(pdb_t* pdb)
{
	IDiaSymbol* pGlobal = pdb->g_pGlobalSymbol;

  wprintf(L"\n\n** *PUBLICS\n\n");

  // Retrieve all the public symbols

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagPublicSymbol, NULL, nsNone, &pEnumSymbols))) {
    return false;
  }

  IDiaSymbol *pSymbol;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
    pdb_public_t* public = (pdb_public_t*)malloc(sizeof(pdb_public_t));
    if (public == NULL) {
      return false;
    }
    memset(public, 0, sizeof(pdb_public_t));

    PrintPublicSymbol(pSymbol, public);

    pSymbol->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// Dump all the symbol information stored in the compilands
//

typedef struct _pdb_symbol 
{
  wchar_t   name[NAME_SIZE];
  uint32_t  dwSymTag;
}pdb_symbol_t;

bool dump_symbols(pdb_t* pdb)
{
	IDiaSymbol* pGlobal = pdb->g_pGlobalSymbol;
  wprintf(L"\n\n*** SYMBOLS\n\n\n");

  // Retrieve the compilands first

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
    return false;
  }

  IDiaSymbol *pCompiland;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
    wprintf(L"\n** Module: ");

    pdb_symbol_t* symbol = (pdb_symbol_t*)malloc(sizeof(pdb_symbol_t));
    if (symbol == NULL) {
      return false;
    }
    memset(symbol, 0, sizeof(pdb_symbol_t));

    // Retrieve the name of the module

    BSTR bstrName;

    if (pCompiland->get_name(&bstrName) != S_OK) {
      wprintf(L"(???)\n\n");
      wcsncpy(symbol->name, bstrName, NAME_SIZE - 1);
    }

    else {
      wprintf(L"%s\n\n", bstrName);
      wcsncpy(symbol->name, bstrName, NAME_SIZE - 1);
      SysFreeString(bstrName);
    }

    // Find all the symbols defined in this compiland and print their info

    IDiaEnumSymbols *pEnumChildren;

    if (SUCCEEDED(pCompiland->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren))) {
      IDiaSymbol *pSymbol;
      ULONG celtChildren = 0;

      while (SUCCEEDED(pEnumChildren->Next(1, &pSymbol, &celtChildren)) && (celtChildren == 1)) {
        PrintSymbol(pSymbol, 0, symbol);
        pSymbol->Release();
      }

      pEnumChildren->Release();
    }

    pCompiland->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}


////////////////////////////////////////////////////////////
// Dump all the global symbols - SymTagFunction,
//  SymTagThunk and SymTagData
//
typedef struct _pdb_global {
  uint32_t  dwSymTag;
  uint32_t  dwRVA;
  uint32_t  dwSeg;
  uint32_t  dwOff;
}pdb_global_t;

bool dump_globals(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
  IDiaEnumSymbols *pEnumSymbols;
  IDiaSymbol *pSymbol;
  enum SymTagEnum dwSymTags[] = { SymTagFunction, SymTagThunk, SymTagData };
  ULONG celt = 0;

  wprintf(L"\n\n*** GLOBALS\n\n");

  for (size_t i = 0; i < _countof(dwSymTags); i++, pEnumSymbols = NULL) {
    if (SUCCEEDED(pGlobal->findChildren(dwSymTags[i], NULL, nsNone, &pEnumSymbols))) {
      while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
        pdb_global_t* global = (pdb_global_t*)malloc(sizeof(pdb_global_t));
        if (global == NULL) {
          return false;
        }
        memset(global, 0, sizeof(pdb_global_t));

        PrintGlobalSymbol(pSymbol, global);

        pSymbol->Release();
      }

      pEnumSymbols->Release();
    }

    else {
      wprintf(L"ERROR - DumpAllGlobals() returned no symbols\n");

      return false;
    }
  }

  putwchar(L'\n');

  return true;
}


////////////////////////////////////////////////////////////
// Dump all the user defined types (UDT)
//
typedef struct _pdb_udt{
  uint32_t dwSymTag;
}pdb_udt_t;

bool dump_udts(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
  wprintf(L"\n\n** User Defined Types\n\n");

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagUDT, NULL, nsNone, &pEnumSymbols))) {
    wprintf(L"ERROR - DumpAllUDTs() returned no symbols\n");

    return false;
  }

  IDiaSymbol *pSymbol;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
    pdb_udt_t* udt = malloc(sizeof(pdb_udt_t));
    if (udt == NULL) {
      return false;
    }
    memset(udt, 0, sizeof(pdb_udt_t));

    PrintTypeInDetail(pSymbol, 0, udt);

    pSymbol->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}


////////////////////////////////////////////////////////////
// Dump all the enum types from the pdb
//
typedef struct _pdb_enum
{

}pdb_enum_t;
bool dump_enums(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
  wprintf(L"\n\n** ENUMS\n\n");

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagEnum, NULL, nsNone, &pEnumSymbols))) {
    wprintf(L"ERROR - DumpAllEnums() returned no symbols\n");

    return false;
  }

  IDiaSymbol *pSymbol;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
    pdb_enum_t* pdb_enum = (pdb_enum_t*)malloc(sizeof(pdb_enum_t));
    if (pdb_enum == NULL) {
      return false;
    }
    memset(pdb_enum, 0, sizeof(pdb_enum_t));

    PrintTypeInDetail(pSymbol, 0, pdb_enum);

    pSymbol->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}


////////////////////////////////////////////////////////////
// Dump all the typedef types from the pdb
//
typedef struct _pdb_typedef
{

}pdb_typedef_t;

bool dump_typedefs(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
  wprintf(L"\n\n** TYPEDEFS\n\n");

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagTypedef, NULL, nsNone, &pEnumSymbols))) {
    wprintf(L"ERROR - DumpAllTypedefs() returned no symbols\n");

    return false;
  }

  IDiaSymbol *pSymbol;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
    pdb_typedef_t* pdb_typedef = (pdb_typedef_t*)malloc(sizeof(pdb_typedef_t));
    if (pdb_typedef == NULL) {
      return false;
    }
    memset(pdb_typedef, 0, sizeof(pdb_typedef_t));
    PrintTypeInDetail(pSymbol, 0, pdb_typedef);

    pSymbol->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// Dump all the types information
//  (type symbols can be UDTs, enums or typedefs)
//
bool dump_types(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
  wprintf(L"\n\n*** TYPES\n");

  return dump_udts(pdb) || dump_enums(pdb) || dump_typedefs(pdb);
}


////////////////////////////////////////////////////////////
// Dump OEM specific types
//

typedef struct _pdb_oem
{

}pdb_oem_t;

bool dump_oems(pdb_t* pdb)
{
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;

  wprintf(L"\n\n*** OEM Specific types\n\n");

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagCustomType, NULL, nsNone, &pEnumSymbols))) {
    wprintf(L"ERROR - DumpAllOEMs() returned no symbols\n");

    return false;
  }

  IDiaSymbol *pSymbol;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && (celt == 1)) {
    pdb_oem_t* pdb_oem = (pdb_oem_t*)malloc(sizeof(pdb_oem_t));
    if (pdb_oem == NULL) {
      return false;
    }
    memset(pdb_oem, 0, sizeof(pdb_oem_t));

    PrintTypeInDetail(pSymbol, 0, pdb_oem);

    pSymbol->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// For each compiland in the PDB dump all the source files
//

typedef struct _pdb_file
{

}pdb_file_t;
bool dump_files(pdb_t* pdb)
{
	IDiaSession *pSession = pdb->g_pDiaSession;
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;

  wprintf(L"\n\n*** FILES\n\n");

  // In order to find the source files, we have to look at the image's compilands/modules

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
    return false;
  }

  IDiaSymbol *pCompiland;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
    BSTR bstrName;

    if (pCompiland->get_name(&bstrName) == S_OK) {
      wprintf(L"\nCompiland = %s\n\n", bstrName);

      SysFreeString(bstrName);
    }

    // Every compiland could contain multiple references to the source files which were used to build it
    // Retrieve all source files by compiland by passing NULL for the name of the source file

    IDiaEnumSourceFiles *pEnumSourceFiles;

    if (SUCCEEDED(pSession->findFile(pCompiland, NULL, nsNone, &pEnumSourceFiles))) {
      IDiaSourceFile *pSourceFile;

      while (SUCCEEDED(pEnumSourceFiles->Next(1, &pSourceFile, &celt)) && (celt == 1)) {
        pdb_file_t* pdb_file = (pdb_file_t*)malloc(sizeof(pdb_file_t));
        if (pdb_file == NULL) {
          return false;
        }
        memset(pdb_file, 0, sizeof(pdb_file_t));
        PrintSourceFile(pSourceFile, pdb_file);
        putwchar(L'\n');

        pSourceFile->Release();
      }

      pEnumSourceFiles->Release();
    }

    putwchar(L'\n');

    pCompiland->Release();
  }

  pEnumSymbols->Release();

  return true;
}

////////////////////////////////////////////////////////////
// Dump all the line numbering information contained in the PDB
//  Only function symbols have corresponding line numbering information

typedef struct _pdb_line
{

}pdb_line_t;

bool dump_lines(pdb_t* pdb)
{
	IDiaSession *pSession = pdb->g_pDiaSession;
	IDiaSymbol *pGlobal = pdb->g_pGlobalSymbol;
  wprintf(L"\n\n*** LINES\n\n");

  // First retrieve the compilands/modules

  IDiaEnumSymbols *pEnumSymbols;

  if (FAILED(pGlobal->findChildren(SymTagCompiland, NULL, nsNone, &pEnumSymbols))) {
    return false;
  }

  IDiaSymbol *pCompiland;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSymbols->Next(1, &pCompiland, &celt)) && (celt == 1)) {
    IDiaEnumSymbols *pEnumFunction;

    // For every function symbol defined in the compiland, retrieve and print the line numbering info

    if (SUCCEEDED(pCompiland->findChildren(SymTagFunction, NULL, nsNone, &pEnumFunction))) {
      IDiaSymbol *pFunction;
      ULONG celt = 0;

      while (SUCCEEDED(pEnumFunction->Next(1, &pFunction, &celt)) && (celt == 1)) {
        pdb_line_t* pdb_line = (pdb_line_t*)malloc(sizeof(pdb_line_t));
        if (pdb_line == NULL) {
          return false;
        }
        memset(pdb_line, 0, sizeof(pdb_line_t));
        PrintLines(pSession, pFunction, pdb_line);

        pFunction->Release();
      }

      pEnumFunction->Release();
    }

    pCompiland->Release();
  }

  pEnumSymbols->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// Dump all the section contributions from the PDB
//
//  Section contributions are stored in a table which will
//  be retrieved via IDiaSession->getEnumTables through
//  QueryInterface()using the REFIID of the IDiaEnumSectionContribs
//
typedef struct _pdb_section_contrib
{

}pdb_section_contrib_t;

bool dump_sec_contribs(pdb_t* pdb)
{
	IDiaSession *pSession = pdb->g_pDiaSession;
  wprintf(L"\n\n*** SECTION CONTRIBUTION\n\n");

  IDiaEnumSectionContribs *pEnumSecContribs;

  if (FAILED(GetTable(pSession, __uuidof(IDiaEnumSectionContribs), (void **) &pEnumSecContribs))) {
    return false;
  }

  wprintf(L"    RVA        Address       Size    Module\n");

  IDiaSectionContrib *pSecContrib;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumSecContribs->Next(1, &pSecContrib, &celt)) && (celt == 1)) {
      pdb_section_contrib_t* pdb_contrib = (pdb_section_contrib_t*)malloc(sizeof(pdb_section_contrib_t));
      if (pdb_contrib == NULL) {
        return false;
      }
      memset(pdb_contrib, 0, sizeof(pdb_section_contrib_t));
    PrintSecContribs(pSecContrib, pdb_contrib);

    pSecContrib->Release();
  }

  pEnumSecContribs->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// Dump all debug data streams contained in the PDB
//
typedef struct _pdb_debug_stream
{

}pdb_debug_stream_t;

bool dump_debug_streams(pdb_t* pdb)
{
	IDiaSession *pSession = pdb->g_pDiaSession;
  IDiaEnumDebugStreams *pEnumStreams;

  wprintf(L"\n\n*** DEBUG STREAMS\n\n");

  // Retrieve an enumerated sequence of debug data streams

  if (FAILED(pSession->getEnumDebugStreams(&pEnumStreams))) {
    return false;
  }

  IDiaEnumDebugStreamData *pStream;
  ULONG celt = 0;

  for (; SUCCEEDED(pEnumStreams->Next(1, &pStream, &celt)) && (celt == 1); pStream = NULL) {
      pdb_debug_stream_t* pdb_debug_stream = (pdb_debug_stream_t*)malloc(sizeof(pdb_debug_stream_t));
      if (pdb_debug_stream == NULL) {
        return false;
      }
      memset(pdb_debug_stream, 0, sizeof(pdb_debug_stream_t));
    PrintStreamData(pStream, pdb_debug_stream);

    pStream->Release();
  }

  pEnumStreams->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// Dump all the injected source from the PDB
//
//  Injected sources data is stored in a table which will
//  be retrieved via IDiaSession->getEnumTables through
//  QueryInterface()using the REFIID of the IDiaEnumSectionContribs
//
typedef struct _pdb_injected_source
{

}pdb_injected_source_t;

bool dump_injected_sources(pdb_t* pdb)
{
	IDiaSession *pSession = pdb->g_pDiaSession;

  wprintf(L"\n\n*** INJECTED SOURCES TABLE\n\n");

  IDiaEnumInjectedSources *pEnumInjSources;

  if (SUCCEEDED(GetTable(pSession, __uuidof(IDiaEnumInjectedSources), (void **) &pEnumInjSources))) {
    return false;
  }

  IDiaInjectedSource *pInjSource;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumInjSources->Next(1, &pInjSource, &celt)) && (celt == 1)) {
      pdb_injected_source_t* pdb_injected_source = (pdb_injected_source_t*)malloc(sizeof(pdb_injected_source_t));
      if (pdb_injected_source == NULL) {
        return false;
      }
      memset(pdb_injected_source, 0, sizeof(pdb_injected_source_t));
    PrintGeneric(pInjSource, pdb_injected_source);

    pInjSource->Release();
  }

  pEnumInjSources->Release();

  putwchar(L'\n');

  return true;
}

////////////////////////////////////////////////////////////
// Dump all the FPO info
//
//  FPO data stored in a table which will be retrieved via
//    IDiaSession->getEnumTables through QueryInterface()
//    using the REFIID of the IDiaEnumFrameData
//
typedef struct _pdb_fpo
{

}pdb_fpo_t;

bool dump_fpo(pdb_t * pdb)
{
	IDiaSession *pSession = pdb->g_pDiaSession;
  IDiaEnumFrameData *pEnumFrameData;

  wprintf(L"\n\n*** FPO\n\n");

  if (FAILED(GetTable(pSession, __uuidof(IDiaEnumFrameData), (void **) &pEnumFrameData))) {
    return false;
  }

  IDiaFrameData *pFrameData;
  ULONG celt = 0;

  while (SUCCEEDED(pEnumFrameData->Next(1, &pFrameData, &celt)) && (celt == 1)) {
      pdb_fpo_t* pdb_fpo = (pdb_fpo_t*)malloc(sizeof(pdb_fpo_t));
      if (pdb_fpo == NULL) {
        return false;
      }
      memset(pdb_fpo, 0, sizeof(pdb_fpo_t));
    PrintFrameData(pFrameData, pdb_fpo);

    pFrameData->Release();
  }

  pEnumFrameData->Release();

  putwchar(L'\n');

  return true;
}

int pdb_load(const wchar_t* pdb_file)
{
	pdb_t* pdb = (pdb_t*)malloc(sizeof(pdb_t));
	if (pdb == NULL) {
		return INVALID_PDB;
	}
	memset(pdb, 0, sizeof(pdb_t));

  char wszExt[MAX_PATH] = {0};
  char *wszSearchPath = "SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data


  HRESULT hr = CoInitialize(NULL);
  // Obtain access to the provider
  hr = CoCreateInstance(__uuidof(DiaSource),
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        __uuidof(IDiaDataSource),
                        (void **) &pdb->g_pDiaDataSource);

  if (FAILED(hr)) {
    printf("CoCreateInstance failed - HRESULT = %08X\n", hr);
    return INVALID_PDB;
  }

  _splitpath_s(pdb_file, NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH);
  if (!stricmp(wszExt, ".pdb")) {
    // Open and prepare a program database (.pdb) file as a debug data source
    hr = pdb->g_pDiaDataSource->loadDataFromPdb(pdb_file);
    if (FAILED(hr)) {
      printf("loadDataFromPdb failed - HRESULT = %08X\n", hr);
      return INVALID_PDB;
    }
  } else {
    CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
                        // thus enabling a user interface to report on the progress of
                        // the location attempt. The client application may optionally
                        // provide a reference to its own implementation of this
                        // virtual base class to the IDiaDataSource::loadDataForExe method.
    callback.AddRef();

    // Open and prepare the debug data associated with the executable
    hr = pdb->g_pDiaDataSource->loadDataForExe(pdb_file, wszSearchPath, &callback);
    if (FAILED(hr)) {
      printf("loadDataForExe failed - HRESULT = %08X\n", hr);
      return INVALID_PDB;
    }
  }

  // Open a session for querying symbols
  hr = pdb->g_pDiaDataSource->openSession(&pdb->g_pDiaSession);
  if (FAILED(hr)) {
    printf("openSession failed - HRESULT = %08X\n", hr);
    return INVALID_PDB;
  }

  // Retrieve a reference to the global scope
  hr = pdb->g_pDiaDataSource->get_globalScope(&pdb->g_pGlobalSymbol);

  if (hr != S_OK) {
    printf("get_globalScope failed\n");
    return INVALID_PDB;
  }

  // Set Machine type for getting correct register names

  DWORD dwMachType = 0;
  if (pdb->g_pGlobalSymbol->get_machineType(&dwMachType) == S_OK) {
    switch (dwMachType) {
      case IMAGE_FILE_MACHINE_I386 : pdb->g_dwMachineType = CV_CFL_80386; break;
      case IMAGE_FILE_MACHINE_IA64 : pdb->g_dwMachineType = CV_CFL_IA64; break;
      case IMAGE_FILE_MACHINE_AMD64 :pdb->g_dwMachineType = CV_CFL_AMD64; break;
    }
  }

	dump_modules(pdb);
	dump_publics(pdb);
	dump_symbols(pdb);
	dump_globals(pdb);
	dump_types(pdb);
	dump_files(pdb);
	dump_lines(pdb);
	dump_sec_contribs(pdb);
	dump_debug_streams(pdb);
	dump_injected_sources(pdb);
	dump_fpo(pdb);
	dump_oems(pdb);

	return (intptr_t)pdb;
}

void pdb_close(int fd)
{

}
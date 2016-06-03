#include <windows.h>
#include <fstream>

#include "vtable/vmthooks.h"

#include "witcher3-classes.h"

#include <iostream>

#include "Logger.h"

#include "utils.h"

const char* SOFTWARE_VERSION = "1.21";

HANDLE thread = nullptr;
utils::VtableHook* game_hook = nullptr;
void** global_debug_console = nullptr;
CRTTISystem* rtti_system = nullptr;
void* native_globals_function_map = nullptr;
CGame** global_game = nullptr;
CLogger* logger = nullptr;

typedef bool (*OnViewportInputType)(void* thisptr,
                                    void* viewport,
                                    EInputKey input_key,
                                    EInputAction input_action,
                                    float tick);
OnViewportInputType OnViewportInputDebugConsole = nullptr;

bool OnViewportInputDebugAlwaysHook(void* thisptr,
                                    void* viewport,
                                    EInputKey input_key,
                                    EInputAction input_action,
                                    float tick) {
  if ((*global_game)->ProcessFreeCameraInput(input_key, input_action, tick))
    return true;

  if (input_key == IK_F2 && input_action == IACT_Release) 
  {
    input_key = IK_Tilde;
    input_action = IACT_Press;
  }

  if (input_key == IK_Backslash && input_action == IACT_Release) 
  {
	  input_key = IK_Tilde;
	  input_action = IACT_Press;
  }

  return OnViewportInputDebugConsole(*global_debug_console, viewport, input_key,
                                     input_action, tick);
}


std::wstring GetExecutablePath() {
  wchar_t buffer[MAX_PATH];
  GetModuleFileName(NULL, buffer, MAX_PATH);
  std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
  return std::wstring(buffer).substr(0, pos);
}

std::vector<std::wstring> GetAllFileNamesFromFolder(std::wstring folder) {
  std::vector<std::wstring> names;
  wchar_t search_path[200];
  wsprintf(search_path, L"%s*.*", folder.c_str());
  WIN32_FIND_DATA fd;
  HANDLE hFind = ::FindFirstFile(search_path, &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        names.push_back(fd.cFileName);
      }
    } while (::FindNextFile(hFind, &fd));
    ::FindClose(hFind);
  }
  return names;
}

static bool(__thiscall* OrginalBaseEngine_InitializeScripts)(void*);

static bool BaseEngine_InitializeScripts(void* rcx, void* rdx) {
  std::wcout << "BaseEngine_InitializeScripts" << std::endl;
  return OrginalBaseEngine_InitializeScripts(rcx);
}

DWORD WINAPI InitializeHook(void* arguments) {
  hook::set_base();
  HookFunction::RunAll();
  logger = new CLogger;

  global_game = hook::pattern("48 8B 05 ? ? ? 01 C6 44 24 30 01 89 4C 24 28")
                    .count(1)
                    .get(0)
                    .extract<CGame**>(3);
  global_debug_console = hook::pattern("48 89 05 ? ? ? ? EB 07 48 89 35 ? ? ? ? 48 8B")
          .count(1)
          .get(0)
          .extract<void**>(3);

  logger->Log("=====================\n Witcher 3 console enabler originally created by Karl Skomski. \n\n\n Modified by Hugo Holmqvist\n \n Credits to Sarcen for the fix for 1.21! \n =====================");

  std::string w3Version = hook::pattern("48 8D 05 4E 75 7E 01").count(1).get(0).extract<char*>(3);
  if (w3Version != "")
  {
	  w3Version = utils::Split(std::string(w3Version), ' ')[1];

	  logger->Log("-> For version "+std::string(SOFTWARE_VERSION)+" (current version "+ w3Version +")");

	  if (w3Version != std::string(SOFTWARE_VERSION))
	  {
		  std::string err = "You are running version " + w3Version + " but this software is for " + std::string(SOFTWARE_VERSION)+". The debug console might not work!";
		  MessageBoxA(NULL, err.c_str(), "Debug Console: Wrong version", MB_OK | MB_ICONEXCLAMATION);
	  }
  }

  while (*global_debug_console == nullptr) {
    Sleep(500);
  }

  OnViewportInputDebugConsole = hook::pattern("48 83 EC 28 48 8B 05 ? ? ? ? 0F B6 90")
          .count(1)
          .get(0)
          .get<OnViewportInputType>(0);

  if (global_game)
	  logger->Log("Game ptr valid.");

  if (global_debug_console)
	  logger->Log("Console ptr valid.");

  if (OnViewportInputDebugConsole)
	  logger->Log("Input ptr valid.");

  game_hook = new utils::VtableHook(*global_game);
  logger->Log("CGame has " + std::to_string(game_hook->NumFuncs()) + " virtual funcs");
  game_hook->HookMethod(OnViewportInputDebugAlwaysHook, 136);

  return 1;
}

void FinalizeHook() {
  if (game_hook)
    delete game_hook;

  if (logger)
	  delete logger;
}

int WINAPI DllMain(HINSTANCE instance, DWORD reason, PVOID reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
	//Sleep(1000);
    thread = CreateThread(nullptr, 0, InitializeHook, 0, 0, nullptr);
  } else if (reason == DLL_PROCESS_DETACH) {
    FinalizeHook();
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
  }
  return 1;
}

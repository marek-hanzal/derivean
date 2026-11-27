// The addon imports Node-API from an import library that names node.exe, and the linker delays
// that import. Electron loads the addon into electron.exe, which exports the same functions, so
// the delay-load hook answers node.exe with the executable that loaded us, whatever its name.
// Built only on Windows (@napi/xmake.lua); node-gyp's win_delay_load_hook.cc does the same (docs/credits/libraries.md).
#include <windows.h>

#include <delayimp.h>
#include <string.h>

namespace {

FARPROC WINAPI bindHost(unsigned event, PDelayLoadInfo info) {
    if (event != dliNotePreLoadLibrary || _stricmp(info->szDll, "node.exe") != 0) return nullptr;
    return reinterpret_cast<FARPROC>(GetModuleHandleW(nullptr));
}

} // namespace

extern "C" const PfnDliHook __pfnDliNotifyHook2 = bindHost;

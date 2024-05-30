#include <d3d9.h>
#include <Windows.h>
#include <winsock.h>
#include <fstream>
#include <kthook/kthook.hpp>

using GameInitSignature = void(__cdecl*)();
using CreateOffscreenPlainSurfaceSignature = HRESULT(__stdcall*)(IDirect3DDevice9 *, UINT, UINT, D3DFORMAT, D3DPOOL, IDirect3DSurface9 **, HANDLE *);

kthook::kthook_simple<GameInitSignature> GameInitHook{0x561B10};
kthook::kthook_simple<CreateOffscreenPlainSurfaceSignature> CursorHook{};

void get_png_image_dimensions(std::string& file_path, unsigned int& width, unsigned int& height) // taked from https://stackoverflow.com/questions/5354459/c-how-to-get-the-image-size-of-a-png-file-in-directory
{
    unsigned char buf[8];
    
    std::ifstream in(file_path);
    in.seekg(16);
    in.read(reinterpret_cast<char*>(&buf), 8);

    width = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
    height = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + (buf[7] << 0);
}

HRESULT __stdcall IDirect3DDevice9__CreateOffscreenPlainSurfaceHooked(const decltype(CursorHook)& hook, IDirect3DDevice9 *pDevice, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle) {
    unsigned int width{}, height{};
    std::string path = "./mouse.png";
    get_png_image_dimensions(path, width, height);
    return hook.get_trampoline()(pDevice, width, height, Format, Pool, ppSurface, pSharedHandle);
}

void CTimer__Update(const decltype(GameInitHook)& hook) {
    static bool init{};
    if (!init) {
        DWORD pDevice = *reinterpret_cast<DWORD*>(0xC97C28);
        void** vTable = *reinterpret_cast<void***>(pDevice);
        CursorHook.set_dest(vTable[36]);
        CursorHook.set_cb(IDirect3DDevice9__CreateOffscreenPlainSurfaceHooked);
        CursorHook.install();
        init = { true };
    }

    hook.get_trampoline()();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        GameInitHook.set_cb(&CTimer__Update);
        GameInitHook.install();
        break;
    case DLL_PROCESS_DETACH:
        CursorHook.remove();
        GameInitHook.remove();
        break;
    }
    return TRUE;
}
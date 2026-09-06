#pragma once
#include "include.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace Hooks
{
	uintptr_t present_addr;
	SafetyHookInline present_hook{};

    WNDPROC oWndProc = nullptr;
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    ID3D11RenderTargetView* pMainRenderTargetView = nullptr;
    HWND game_hwnd = nullptr;

    bool init = false;
    bool menu = true;

    LRESULT __stdcall hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
    }

    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
    {
        if (!init)
        {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
            {
                pDevice->GetImmediateContext(&pContext);

                DXGI_SWAP_CHAIN_DESC sd;
                pSwapChain->GetDesc(&sd);
                game_hwnd = sd.OutputWindow;

                ID3D11Texture2D* pBackBuffer = nullptr;
                pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
                if (pBackBuffer) {
                    pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
                    pBackBuffer->Release();
                }

                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO(); (void)io;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

                ImGui_ImplWin32_Init(game_hwnd);
                ImGui_ImplDX11_Init(pDevice, pContext);

                oWndProc = (WNDPROC)SetWindowLongPtr(game_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

                init = true;
            }
        }

        if (GetAsyncKeyState(VK_HOME) & 1) {
            menu = !menu;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (menu)
        {
            ImGui::Begin("Discord Overlay Hook", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            ImGui::Text("Hello! - FPS: %f", ImGui::GetIO().Framerate);
            ImGui::End();
        }

        ImGui::Render();

        pContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        return present_hook.stdcall<HRESULT>(pSwapChain, SyncInterval, Flags);
    }

	void Load()
	{
		Console::Log("Waiting for overlay...");

        bool found_present = false;
        while (!found_present)
        {
            present_addr = Sig::Scan(DISCORD_DLL, DISCORD_PRESENT);
            if (present_addr != 0)
            {
                found_present = true;
                Console::Log("Found Present at: 0x%p", (void*)present_addr);
                present_hook = safetyhook::create_inline(reinterpret_cast<void*>(present_addr), reinterpret_cast<void*>(hkPresent));
            }
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
	}

	void Unload()
	{
		present_hook.reset();

		if (oWndProc) 
        {
			SetWindowLongPtr(game_hwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
		}

		if (init) 
        {
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}

		if (pMainRenderTargetView) { pMainRenderTargetView->Release(); pMainRenderTargetView = nullptr; }
		if (pContext) { pContext->Release(); pContext = nullptr; }
		if (pDevice) { pDevice->Release(); pDevice = nullptr; }
	}
}
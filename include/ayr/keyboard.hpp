#ifndef AYR_KEYBOARD_HPP
#define AYR_KEYBOARD_HPP

#include <functional>

#include "filesystem.hpp"

namespace ayr
{
	namespace keyboard
	{
#if defined(AYR_WINDOWS)
#pragma comment(lib, "User32.lib")

		static std::function<void(int /* keycode*/)> _ON_KEYDOWN, _ON_KEYUP;
		static std::function<bool(int /* keycode*/)> _ON_EXIT;
		static HHOOK _KEYBOARD_HOOK = nullptr;

		// 钩子回调函数
		def CALLBACK _keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam)
		{
			if (nCode == HC_ACTION)
			{ // 只处理有效的键盘事件
				KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
				bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
				bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

				if (isKeyDown || isKeyUp)
				{
					if (_ON_EXIT(kbStruct->vkCode))
						PostQuitMessage(0);
					else if (isKeyDown)
						_ON_KEYDOWN(kbStruct->vkCode);
					else if (isKeyUp)
						_ON_KEYUP(kbStruct->vkCode);
				}
			}

			// 传递事件给下一个钩子
			return CallNextHookEx(nullptr, nCode, wParam, lParam);
		}

		def _init_keyboard(
			const std::function<void(int)>& on_keydown,
			const std::function<void(int)>& on_keyup,
			const std::function<bool(int)>& on_exit
		)
		{
			_ON_KEYDOWN = on_keydown;
			_ON_KEYUP = on_keyup;
			_ON_EXIT = on_exit;

			return SetWindowsHookEx(WH_KEYBOARD_LL, _keyboard_proc, GetModuleHandle(nullptr), 0);
		}


		def listen(
			const std::function<void(int)>& on_keydown,
			const std::function<void(int)>& on_keyup,
			const std::function<bool(int)>& on_exit = [](int keycode) { return keycode == VK_ESCAPE; }
		)
		{
			MSG msg;
			HHOOK hook = _init_keyboard(on_keydown, on_keyup, on_exit);

			while (GetMessage(&msg, nullptr, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			UnhookWindowsHookEx(hook);
		}
#endif // AYR_WINDOS
	}
}
#endif
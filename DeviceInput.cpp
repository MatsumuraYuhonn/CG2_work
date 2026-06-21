#include "DeviceInput.h"
#include <cassert>

HWND hwnd = nullptr;
IDirectInput8* directInput = nullptr;
IDirectInputDevice8* keyboard = nullptr;
BYTE key[256] = {};
BYTE preKey[256] = {};
HRESULT hr;

void DeviceInputInitialize(HINSTANCE hInstance, HWND targetHwnd) {

	hwnd = targetHwnd;

    // 第1引数に main からもらった hInstance を渡す
    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION,
        IID_IDirectInput8, (void**)&directInput, nullptr);
    assert(SUCCEEDED(hr));

    hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    assert(SUCCEEDED(hr));

    hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    // 正しい hwnd がセットされているので成功するようになる
    hr = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));


    memcpy(preKey, key, sizeof(key));
    DeviceInputUpdate();

}

void DeviceInputUpdate() {

    memcpy(preKey, key, sizeof(key));

    if (keyboard) {
        keyboard->Acquire();
        keyboard->GetDeviceState(sizeof(key), key);
    }


}

bool Key(BYTE keyNumber) {
	return key[keyNumber] & 0x80;
}

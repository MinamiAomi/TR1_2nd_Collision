#include "Input.hpp"

#include <dinput.h>
#include <wrl.h>

#include <cstdint>
#include <cstdlib>

#include "Utils.hpp"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")


class Input::Impl {
public:
    void Initialize(HWND hwnd) {
        COM_RESULT(DirectInput8Create(
            GetModuleHandle(nullptr), DIRECTINPUT_HEADER_VERSION,
            IID_IDirectInput8, static_cast<void**>(&direct_input_), nullptr));

        COM_RESULT(direct_input_->CreateDevice(GUID_SysKeyboard, &keybord_, nullptr));
        COM_RESULT(keybord_->SetDataFormat(&c_dfDIKeyboard));
        COM_RESULT(keybord_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY));

    }


private:
    static constexpr uint32_t kKeyCount = 256;

    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<IDirectInput8> direct_input_;
    ComPtr<IDirectInputDevice8> keybord_;
    uint8_t keys_[kKeyCount];
    uint8_t pre_keys_[kKeyCount];
};

void Input::Initialize(HWND hwnd) {
    if (!hwnd) {
        hwnd = GetActiveWindow();
    }
    GetInstance()->pimpl_->Initialize(hwnd);
}

Input* Input::GetInstance() {
    static Input instance;
    return &instance;
}

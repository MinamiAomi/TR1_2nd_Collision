#include "Input.hpp"

#include <dinput.h>
#include <wrl.h>

#include <cstdint>

#pragma comment(lib,"dinput8.lib")

class Input::Impl {
public:
    void Initialize() {
        HRESULT result = DirectInput8Create(
            GetModuleHandle(nullptr), DIRECTINPUT_HEADER_VERSION,
            IID_IDirectInput8, static_cast<void**>(&direct_input_), nullptr);
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

Input* Input::GetInstance() {
    static Input instance;
    return &instance;
}

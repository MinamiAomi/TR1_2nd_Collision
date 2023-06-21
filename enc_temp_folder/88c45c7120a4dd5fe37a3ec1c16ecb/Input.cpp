#include "Input.hpp"

#include <dinput.h>

#pragma comment(lib,"dinput8.lib")

class Input::Impl {

};

Input* Input::GetInstance() {
    static Input instance;
    return &instance;
}

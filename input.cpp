#include "input.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace {

INPUT_STATE get_state(const std::unordered_map<int, INPUT_STATE>& states, int key) {
    auto it = states.find(key);
    if (it == states.end())
        return UP;
    return it->second;
}

INPUT_STATE get_state(const std::unordered_map<SDL_Scancode, INPUT_STATE>& states, SDL_Scancode key) {
    auto it = states.find(key);
    if (it == states.end())
        return UP;
    return it->second;
}

}

void Input::Init() {
    keyboard_states.clear();
    just_down_scancodes.clear();
    just_up_scancodes.clear();
    mouse_position = glm::vec2(0.0f, 0.0f);
    mouse_delta = glm::vec2(0.0f, 0.0f);
    mouse_button_states.clear();
    just_down_buttons.clear();
    just_up_buttons.clear();
    mouse_scroll_this_frame = 0.0f;
}

void Input::ProcessEvent(const SDL_Event &e) {
    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        SDL_Scancode keycode = e.key.keysym.scancode;
        INPUT_STATE previous_state = get_state(keyboard_states, keycode);

        if (e.type == SDL_KEYDOWN) {
            if (previous_state == UP || previous_state == JUST_UP) {
                keyboard_states[keycode] = JUST_DOWN;
                if (std::find(just_down_scancodes.begin(), just_down_scancodes.end(), keycode) == just_down_scancodes.end())
                    just_down_scancodes.emplace_back(keycode);
            } else if (previous_state == JUST_DOWN) {
                keyboard_states[keycode] = JUST_DOWN;
            } else {
                keyboard_states[keycode] = DOWN;
            }
            return;
        }

        if (previous_state == DOWN || previous_state == JUST_DOWN) {
            keyboard_states[keycode] = JUST_UP;
            if (std::find(just_up_scancodes.begin(), just_up_scancodes.end(), keycode) == just_up_scancodes.end())
                just_up_scancodes.emplace_back(keycode);
        } else if (previous_state == JUST_UP) {
            keyboard_states[keycode] = JUST_UP;
        } else {
            keyboard_states[keycode] = UP;
        }
        return;
    }

    if (e.type == SDL_MOUSEMOTION) {
        mouse_position.x = static_cast<float>(e.motion.x);
        mouse_position.y = static_cast<float>(e.motion.y);
        mouse_delta.x += static_cast<float>(e.motion.xrel);
        mouse_delta.y += static_cast<float>(e.motion.yrel);
        return;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
        int button = e.button.button;
        if (button != 1 && button != 2 && button != 3)
            return;

        INPUT_STATE previous_state = get_state(mouse_button_states, button);
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (previous_state == UP || previous_state == JUST_UP) {
                mouse_button_states[button] = JUST_DOWN;
                if (std::find(just_down_buttons.begin(), just_down_buttons.end(), button) == just_down_buttons.end())
                    just_down_buttons.emplace_back(button);
            } else if (previous_state == JUST_DOWN) {
                mouse_button_states[button] = JUST_DOWN;
            } else {
                mouse_button_states[button] = DOWN;
            }
            return;
        }

        if (previous_state == DOWN || previous_state == JUST_DOWN) {
            mouse_button_states[button] = JUST_UP;
            if (std::find(just_up_buttons.begin(), just_up_buttons.end(), button) == just_up_buttons.end())
                just_up_buttons.emplace_back(button);
        } else if (previous_state == JUST_UP) {
            mouse_button_states[button] = JUST_UP;
        } else {
            mouse_button_states[button] = UP;
        }
        return;
    }

    if (e.type == SDL_MOUSEWHEEL)
        mouse_scroll_this_frame += e.wheel.preciseY;
}

void Input::LateUpdate() {
    for (SDL_Scancode keycode : just_down_scancodes) {
        auto it = keyboard_states.find(keycode);
        if (it != keyboard_states.end() && it->second == JUST_DOWN)
            it->second = DOWN;
    }
    for (SDL_Scancode keycode : just_up_scancodes) {
        auto it = keyboard_states.find(keycode);
        if (it != keyboard_states.end() && it->second == JUST_UP)
            it->second = UP;
    }
    for (int button : just_down_buttons) {
        auto it = mouse_button_states.find(button);
        if (it != mouse_button_states.end() && it->second == JUST_DOWN)
            it->second = DOWN;
    }
    for (int button : just_up_buttons) {
        auto it = mouse_button_states.find(button);
        if (it != mouse_button_states.end() && it->second == JUST_UP)
            it->second = UP;
    }
    just_down_scancodes.clear();
    just_up_scancodes.clear();
    just_down_buttons.clear();
    just_up_buttons.clear();
    mouse_scroll_this_frame = 0.0f;
    mouse_delta = glm::vec2(0.0f, 0.0f);
}

bool Input::GetKey(const std::string& keycode) {
    SDL_Scancode scancode = StringToScancode(keycode);
    if (scancode == SDL_SCANCODE_UNKNOWN)
        return false;
    INPUT_STATE state = get_state(keyboard_states, scancode);
    return state == DOWN || state == JUST_DOWN;
}

bool Input::GetKeyDown(const std::string& keycode) {
    SDL_Scancode scancode = StringToScancode(keycode);
    if (scancode == SDL_SCANCODE_UNKNOWN)
        return false;
    return get_state(keyboard_states, scancode) == JUST_DOWN;
}

bool Input::GetKeyUp(const std::string& keycode) {
    SDL_Scancode scancode = StringToScancode(keycode);
    if (scancode == SDL_SCANCODE_UNKNOWN)
        return false;
    return get_state(keyboard_states, scancode) == JUST_UP;
}

glm::vec2 Input::GetMousePosition() {
    return mouse_position;
}

glm::vec2 Input::GetMouseDelta() {
    return mouse_delta;
}

bool Input::GetMouseButton(int button) {
    if (button != 1 && button != 2 && button != 3)
        return false;
    INPUT_STATE state = get_state(mouse_button_states, button);
    return state == DOWN || state == JUST_DOWN;
}

bool Input::GetMouseButtonDown(int button) {
    if (button != 1 && button != 2 && button != 3)
        return false;
    return get_state(mouse_button_states, button) == JUST_DOWN;
}

bool Input::GetMouseButtonUp(int button) {
    if (button != 1 && button != 2 && button != 3)
        return false;
    return get_state(mouse_button_states, button) == JUST_UP;
}

float Input::GetMouseScrollDelta() {
    return mouse_scroll_this_frame;
}

void Input::HideCursor() {
    SDL_ShowCursor(SDL_DISABLE);
}

void Input::ShowCursor() {
    SDL_ShowCursor(SDL_ENABLE);
}

void Input::SetRelativeMouseMode(bool enabled) {
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
}

SDL_Scancode Input::StringToScancode(const std::string& keycode) {
    static const std::unordered_map<std::string, SDL_Scancode> keycode_map = {
        {"up", SDL_SCANCODE_UP},
        {"down", SDL_SCANCODE_DOWN},
        {"left", SDL_SCANCODE_LEFT},
        {"right", SDL_SCANCODE_RIGHT},
        {"space", SDL_SCANCODE_SPACE},
        {"enter", SDL_SCANCODE_RETURN},
        {"return", SDL_SCANCODE_RETURN},
        {"escape", SDL_SCANCODE_ESCAPE},
        {"esc", SDL_SCANCODE_ESCAPE},
        {"tab", SDL_SCANCODE_TAB},
        {"backspace", SDL_SCANCODE_BACKSPACE},
        {"delete", SDL_SCANCODE_DELETE},
        {"insert", SDL_SCANCODE_INSERT},
        {"home", SDL_SCANCODE_HOME},
        {"end", SDL_SCANCODE_END},
        {"page up", SDL_SCANCODE_PAGEUP},
        {"page down", SDL_SCANCODE_PAGEDOWN},
        {"caps lock", SDL_SCANCODE_CAPSLOCK},
        {"left shift", SDL_SCANCODE_LSHIFT},
        {"right shift", SDL_SCANCODE_RSHIFT},
        {"lshift", SDL_SCANCODE_LSHIFT},
        {"rshift", SDL_SCANCODE_RSHIFT},
        {"left ctrl", SDL_SCANCODE_LCTRL},
        {"right ctrl", SDL_SCANCODE_RCTRL},
        {"lctrl", SDL_SCANCODE_LCTRL},
        {"rctrl", SDL_SCANCODE_RCTRL},
        {"left alt", SDL_SCANCODE_LALT},
        {"right alt", SDL_SCANCODE_RALT},
        {"lalt", SDL_SCANCODE_LALT},
        {"ralt", SDL_SCANCODE_RALT},
        {"left gui", SDL_SCANCODE_LGUI},
        {"right gui", SDL_SCANCODE_RGUI},
        {"lgui", SDL_SCANCODE_LGUI},
        {"rgui", SDL_SCANCODE_RGUI},
        {"/", SDL_SCANCODE_SLASH},
        {";", SDL_SCANCODE_SEMICOLON},
        {"=", SDL_SCANCODE_EQUALS},
        {"-", SDL_SCANCODE_MINUS},
        {".", SDL_SCANCODE_PERIOD},
        {",", SDL_SCANCODE_COMMA},
        {"[", SDL_SCANCODE_LEFTBRACKET},
        {"]", SDL_SCANCODE_RIGHTBRACKET},
        {"\\", SDL_SCANCODE_BACKSLASH},
        {"'", SDL_SCANCODE_APOSTROPHE},
        {"minus", SDL_SCANCODE_MINUS},
        {"equals", SDL_SCANCODE_EQUALS},
        {"comma", SDL_SCANCODE_COMMA},
        {"period", SDL_SCANCODE_PERIOD},
        {"slash", SDL_SCANCODE_SLASH},
        {"semicolon", SDL_SCANCODE_SEMICOLON},
        {"quote", SDL_SCANCODE_APOSTROPHE},
        {"left bracket", SDL_SCANCODE_LEFTBRACKET},
        {"right bracket", SDL_SCANCODE_RIGHTBRACKET},
        {"backslash", SDL_SCANCODE_BACKSLASH},
        {"grave", SDL_SCANCODE_GRAVE}
    };

    std::string normalized;
    normalized.reserve(keycode.size());
    for (unsigned char c : keycode) {
        if (c == '_')
            normalized.push_back(' ');
        else
            normalized.push_back(static_cast<char>(std::tolower(c)));
    }
    auto it = keycode_map.find(normalized);
    if (it != keycode_map.end())
        return it->second;

    SDL_Scancode scancode = SDL_GetScancodeFromName(normalized.c_str());
    if (scancode != SDL_SCANCODE_UNKNOWN)
        return scancode;

    if (normalized.size() == 1) {
        char c = normalized[0];
        if (c >= 'a' && c <= 'z') {
            std::string upper(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            return SDL_GetScancodeFromName(upper.c_str());
        }
        if (c >= '0' && c <= '9') {
            std::string digit(1, c);
            return SDL_GetScancodeFromName(digit.c_str());
        }
    }

    return SDL_SCANCODE_UNKNOWN;
}

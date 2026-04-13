#ifndef INPUT_H
#define INPUT_H

#include "SDL2/SDL.h"
#include "glm.hpp"
#include <string>
#include <unordered_map>
#include <vector>

enum INPUT_STATE { UP, JUST_DOWN, DOWN, JUST_UP };

class Input
{
public:
    static void Init();
    static void ProcessEvent(const SDL_Event & e); // Call every frame at start of event loop.
    static void LateUpdate();

    static bool GetKey(const std::string& keycode); // api
    static bool GetKeyDown(const std::string& keycode); // api
    static bool GetKeyUp(const std::string& keycode); // api

    static glm::vec2 GetMousePosition(); // api
    static glm::vec2 GetMouseDelta(); // api
    static bool GetMouseButton(int button); // api
    static bool GetMouseButtonDown(int button); // api
    static bool GetMouseButtonUp(int button); // api
    static float GetMouseScrollDelta(); // api
    static void HideCursor(); // api
    static void ShowCursor(); // api
    static void SetRelativeMouseMode(bool enabled); // api 3d

private:
    static SDL_Scancode StringToScancode(const std::string& keycode);

    static inline std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states;
    static inline std::vector<SDL_Scancode> just_down_scancodes;
    static inline std::vector<SDL_Scancode> just_up_scancodes;
    static inline glm::vec2 mouse_position = glm::vec2(0.0f, 0.0f);
    static inline glm::vec2 mouse_delta = glm::vec2(0.0f, 0.0f);
    static inline std::unordered_map<int, INPUT_STATE> mouse_button_states;
    static inline std::vector<int> just_down_buttons;
    static inline std::vector<int> just_up_buttons;
    static inline float mouse_scroll_this_frame = 0.0f;
};

#endif

local function get_camera_state(self)
    return self.actor:GetComponent("CameraController")
end

local function clamp(value, min_value, max_value)
    if value < min_value then
        return min_value
    end
    if value > max_value then
        return max_value
    end
    return value
end

MouseLook = {
    enabled = true,

    OnUpdate = function(self)
        local camera_state = get_camera_state(self)
        if camera_state == nil then
            return
        end

        if Input.GetKeyDown("escape") then
            camera_state:SetMouseCaptured(not camera_state.mouse_captured)
        end

        local mouse_delta = vec2(0.0, 0.0)
        if camera_state.mouse_captured then
            mouse_delta = Input.GetMouseDelta()
        end

        camera_state.yaw_degrees = camera_state.yaw_degrees - mouse_delta.x * camera_state.mouse_sensitivity
        camera_state.pitch_degrees = clamp(camera_state.pitch_degrees - mouse_delta.y * camera_state.mouse_sensitivity, -89.0, 89.0)
    end,

    OnDestroy = function(self)
        local camera_state = get_camera_state(self)
        if camera_state ~= nil then
            camera_state:SetMouseCaptured(false)
        end
    end
}

CameraController = {
    enabled = true,
    mouse_sensitivity = 0.06,
    interaction_distance = 6.0,
    first_person_distance = 0.0,
    first_person_height = 0.0,
    follow_distance = 4.5,
    follow_height = 1.2,
    third_person_enabled = true,
    camera = nil,
    aim_transform = nil,
    yaw_degrees = 0.0,
    pitch_degrees = 0.0,
    mouse_captured = true,

    SetMouseCaptured = function(self, captured)
        self.mouse_captured = captured
        if captured then
            Input.HideCursor()
            if Input.SetRelativeMouseMode ~= nil then
                Input.SetRelativeMouseMode(true)
            end
        else
            if Input.SetRelativeMouseMode ~= nil then
                Input.SetRelativeMouseMode(false)
            end
            Input.ShowCursor()
        end
    end,

    GetLookDirection = function(self)
        self.aim_transform.rotation_degrees = Vector3(self.pitch_degrees, self.yaw_degrees, 0.0)
        return self.aim_transform:GetForwardVector()
    end,

    TogglePerspective = function(self)
        self.third_person_enabled = not self.third_person_enabled
    end,

    OnStart = function(self)
        self.camera = Camera3D()
        self.aim_transform = Transform3D()

        self.camera.field_of_view_degrees = 70.0
        self.camera.near_clip = 0.1
        self.camera.far_clip = 250.0

        self:SetMouseCaptured(true)
    end,

    OnUpdate = function(self)
        if Input.GetKeyDown("e") then
            self:TogglePerspective()
        end
    end,

    OnDestroy = function(self)
        self:SetMouseCaptured(false)
    end
}

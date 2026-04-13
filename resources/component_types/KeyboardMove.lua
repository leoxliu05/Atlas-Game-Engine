local function get_player_state(self)
    return self.actor:GetComponent("PlayerController")
end

local function get_camera_state()
    local camera_actor = Actor.Find("camera")
    if camera_actor == nil then
        return nil
    end
    return camera_actor:GetComponent("CameraController")
end

local function move_towards(current, target, max_delta)
    local delta = Vector3.Subtract(target, current)
    local distance = Vector3.Length(delta)
    if distance <= max_delta or distance <= 0.0001 then
        return target
    end
    return Vector3.Add(current, Vector3.Multiply(Vector3.Normalize(delta), max_delta))
end

local function normalize_angle(angle)
    while angle > 180.0 do
        angle = angle - 360.0
    end
    while angle < -180.0 do
        angle = angle + 360.0
    end
    return angle
end

local function move_angle_towards(current, target, max_delta)
    local delta = normalize_angle(target - current)
    if math.abs(delta) <= max_delta then
        return target
    end
    if delta > 0.0 then
        return normalize_angle(current + max_delta)
    end
    return normalize_angle(current - max_delta)
end

local function yaw_from_direction(direction)
    if math.atan2 ~= nil then
        return math.deg(math.atan2(direction.x, -direction.z))
    end
    return math.deg(math.atan(direction.x, -direction.z))
end

KeyboardMove = {
    enabled = true,

    OnUpdate = function(self)
        local player_state = get_player_state(self)
        local camera_state = get_camera_state()
        if player_state == nil or camera_state == nil then
            return
        end

        local dt = Application.GetDeltaTime()

        player_state.movement_transform.rotation_degrees = Vector3(0.0, camera_state.yaw_degrees, 0.0)
        local forward = player_state.movement_transform:GetForwardVector()
        local right = player_state.movement_transform:GetRightVector()

        local move = Vector3(0.0, 0.0, 0.0)
        if Input.GetKey("w") then
            move = Vector3.Add(move, forward)
        end
        if Input.GetKey("s") then
            move = Vector3.Subtract(move, forward)
        end
        if Input.GetKey("d") then
            move = Vector3.Add(move, right)
        end
        if Input.GetKey("a") then
            move = Vector3.Subtract(move, right)
        end

        local desired_horizontal = Vector3(0.0, 0.0, 0.0)
        if Vector3.Length(move) > 0.0 then
            desired_horizontal = Vector3.Multiply(Vector3.Normalize(move), player_state.move_speed)
        end

        local accel = player_state.ground_deceleration
        if Vector3.Length(desired_horizontal) > 0.0 then
            accel = player_state.ground_acceleration
            if not player_state.is_grounded then
                accel = player_state.air_acceleration
            end
        elseif not player_state.is_grounded then
            accel = player_state.air_deceleration
        end

        player_state.horizontal_velocity = move_towards(
            player_state.horizontal_velocity,
            desired_horizontal,
            accel * dt
        )

        player_state.horizontal_velocity = player_state:MoveHorizontally(
            player_state.horizontal_velocity,
            dt
        )

        player_state.visual_yaw_degrees = camera_state.yaw_degrees
        player_state.head_yaw_degrees = camera_state.yaw_degrees
        player_state.head_pitch_degrees = camera_state.pitch_degrees

        local planar_speed = Vector3.Length(player_state.horizontal_velocity)
        if planar_speed > 0.05 then
            local movement_direction = Vector3.Normalize(player_state.horizontal_velocity)
            local target_body_yaw = yaw_from_direction(movement_direction)
            player_state.body_yaw_degrees = move_angle_towards(
                player_state.body_yaw_degrees,
                target_body_yaw,
                540.0 * dt
            )
        end

        if player_state.is_grounded and planar_speed > 0.05 then
            player_state.walk_cycle = player_state.walk_cycle + planar_speed * dt * 4.0
        end

        if player_state.is_grounded and Input.GetKeyDown("space") then
            player_state.vertical_velocity = player_state.jump_speed
            player_state.is_grounded = false
            player_state.desired_ground_y = nil
        end

        player_state.vertical_velocity = player_state.vertical_velocity - player_state.gravity * dt
        player_state.camera_position.y = player_state.camera_position.y + player_state.vertical_velocity * dt

        local ground_height = player_state:GetGroundHeightAt(player_state.camera_position)
        local grounded_eye_y = ground_height + player_state.eye_height
        if player_state.camera_position.y <= grounded_eye_y then
            player_state.desired_ground_y = grounded_eye_y
            if not player_state.is_grounded then
                player_state.camera_position.y = grounded_eye_y
            else
                local rise = grounded_eye_y > player_state.camera_position.y
                local follow_speed = player_state.ground_follow_speed
                if rise then
                    follow_speed = player_state.step_rise_speed
                end
                player_state.camera_position.y = math.min(
                    player_state.camera_position.y + follow_speed * dt,
                    grounded_eye_y
                )
            end
            player_state.vertical_velocity = 0.0
            player_state.is_grounded = true
        elseif player_state.is_grounded and player_state.desired_ground_y ~= nil then
            local next_y = player_state.camera_position.y
            if next_y < player_state.desired_ground_y then
                next_y = math.min(next_y + player_state.ground_follow_speed * dt, player_state.desired_ground_y)
            else
                next_y = math.max(next_y - player_state.ground_follow_speed * dt, player_state.desired_ground_y)
            end
            player_state.camera_position.y = next_y
            if math.abs(player_state.camera_position.y - player_state.desired_ground_y) <= 0.001 then
                player_state.camera_position.y = player_state.desired_ground_y
            end
        else
            player_state.desired_ground_y = nil
            player_state.is_grounded = false
        end

        if player_state:IsPlayerCollidingAt(player_state.camera_position) then
            player_state.camera_position.y = player_state.camera_position.y - player_state.vertical_velocity * dt
            player_state.vertical_velocity = 0.0
        end
    end
}

local function get_camera_state(self)
    return self.actor:GetComponent("CameraController")
end

local function get_player_state()
    local player_actor = Actor.Find("player")
    if player_actor == nil then
        return nil
    end
    return player_actor:GetComponent("PlayerController")
end

local function get_world_state()
    local world_actor = Actor.Find("world")
    if world_actor == nil then
        return nil
    end
    return world_actor:GetComponent("BlockWorld")
end

local function round_to_block(value)
    return math.floor(value + 0.5)
end

local function dominant_axis_normal(normal)
    local abs_x = math.abs(normal.x)
    local abs_y = math.abs(normal.y)
    local abs_z = math.abs(normal.z)

    if abs_x >= abs_y and abs_x >= abs_z then
        if normal.x >= 0.0 then
            return Vector3(1.0, 0.0, 0.0)
        end
        return Vector3(-1.0, 0.0, 0.0)
    end
    if abs_y >= abs_z then
        if normal.y >= 0.0 then
            return Vector3(0.0, 1.0, 0.0)
        end
        return Vector3(0.0, -1.0, 0.0)
    end
    if normal.z >= 0.0 then
        return Vector3(0.0, 0.0, 1.0)
    end
    return Vector3(0.0, 0.0, -1.0)
end

local function get_block_targets_from_raycast(hit)
    if hit == nil or hit.point == nil or hit.normal == nil then
        return nil, nil
    end

    local face_normal = dominant_axis_normal(hit.normal)
    local hit_center = Vector3.Subtract(hit.point, Vector3.Multiply(face_normal, 0.5))
    local place_center = Vector3.Add(hit.point, Vector3.Multiply(face_normal, 0.5))

    local hit_block = {
        x = round_to_block(hit_center.x),
        y = round_to_block(hit_center.y),
        z = round_to_block(hit_center.z)
    }
    local place_block = {
        x = round_to_block(place_center.x),
        y = round_to_block(place_center.y),
        z = round_to_block(place_center.z)
    }
    return hit_block, place_block
end

BlockInteraction = {
    enabled = true,

    OnLateUpdate = function(self)
        local camera_state = get_camera_state(self)
        local player_state = get_player_state()
        local world_state = get_world_state()
        if camera_state == nil or player_state == nil or world_state == nil then
            return
        end

        local look_direction = camera_state:GetLookDirection()
        local ray_origin = camera_state.camera.transform.position
        local ray_distance = camera_state.interaction_distance
        if camera_state.third_person_enabled then
            ray_distance = ray_distance + camera_state.follow_distance
        end

        local raycast = Physics3D.Raycast(
            ray_origin,
            look_direction,
            ray_distance
        )
        local hit_block, place_block = get_block_targets_from_raycast(raycast)

        if Input.GetMouseButtonDown(1) and hit_block ~= nil and world_state:HasBlock(hit_block.x, hit_block.y, hit_block.z) then
            world_state:RemoveBlock(hit_block.x, hit_block.y, hit_block.z)
        end
        if Input.GetMouseButtonDown(3) and place_block ~= nil then
            local target = place_block
            if player_state:CanPlaceBlockAt(world_state, target.x, target.y, target.z) then
                world_state:AddBlock(target.x, target.y, target.z, "placed")
            end
        end
    end
}

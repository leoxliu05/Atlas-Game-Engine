local function get_world_state(self)
    return self.actor:GetComponent("BlockWorld")
end

local function get_actor_component(actor_name, component_name)
    local target_actor = Actor.Find(actor_name)
    if target_actor == nil then
        return nil
    end
    return target_actor:GetComponent(component_name)
end

local function round_to_block(value)
    return math.floor(value + 0.5)
end

local function point_hits_block(world_state, point)
    return world_state:HasBlock(
        round_to_block(point.x),
        round_to_block(point.y),
        round_to_block(point.z)
    )
end

local function resolve_camera_position(world_state, origin, desired)
    local travel = Vector3.Subtract(desired, origin)
    local distance = Vector3.Length(travel)
    if distance <= 0.001 then
        return desired
    end

    local direction = Vector3.Normalize(travel)
    local safe_position = origin
    local step = 0.15
    local cursor = step
    while cursor <= distance do
        local sample = Vector3.Add(origin, Vector3.Multiply(direction, cursor))
        if point_hits_block(world_state, sample) then
            return safe_position
        end
        safe_position = sample
        cursor = cursor + step
    end
    return desired
end

local function submit_cube(world_state, camera, light, position, scale, rotation_degrees, color)
    world_state.render_transform.position = position
    world_state.render_transform.scale = scale
    world_state.render_transform.rotation_degrees = rotation_degrees
    local material = Material3D()
    material.tint = color
    Renderer3D.SubmitMesh(
        world_state.mesh,
        world_state.render_transform,
        camera,
        light,
        material
    )
end

local function rotate_y(offset, yaw_degrees)
    local radians = math.rad(yaw_degrees)
    local cosine = math.cos(radians)
    local sine = math.sin(radians)
    return Vector3(
        offset.x * cosine - offset.z * sine,
        offset.y,
        offset.x * sine + offset.z * cosine
    )
end

local function submit_player_part(world_state, camera, light, anchor, local_offset, scale, rotation_degrees, color)
    submit_cube(
        world_state,
        camera,
        light,
        Vector3.Add(anchor, local_offset),
        scale,
        rotation_degrees,
        color
    )
end

local function render_player(world_state, player_state, camera_state)
    local feet_position = Vector3(
        player_state.camera_position.x,
        player_state.camera_position.y - player_state.eye_height,
        player_state.camera_position.z
    )
    local body_yaw = player_state.body_yaw_degrees
    local head_yaw = player_state.head_yaw_degrees
    local head_pitch = player_state.head_pitch_degrees * 0.35
    local head_color = world_state:GetColor("skin")
    local shirt_color = world_state:GetColor("shirt")
    local pants_color = world_state:GetColor("pants")
    local shoe_color = world_state:GetColor("shoes")

    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        Vector3(0.0, 1.55, 0.0),
        Vector3(0.25, 0.25, 0.25), Vector3(head_pitch, head_yaw, 0.0), head_color)

    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        Vector3(0.0, 0.95, 0.0),
        Vector3(0.21, 0.33, 0.21), Vector3(0.0, body_yaw, 0.0), shirt_color)

    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        rotate_y(Vector3(-0.33, 0.95, 0.0), body_yaw),
        Vector3(0.08, 0.30, 0.08), Vector3(0.0, body_yaw, 0.0), shirt_color)
    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        rotate_y(Vector3(0.33, 0.95, 0.0), body_yaw),
        Vector3(0.08, 0.30, 0.08), Vector3(0.0, body_yaw, 0.0), shirt_color)

    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        rotate_y(Vector3(-0.14, 0.35, 0.0), body_yaw),
        Vector3(0.10, 0.35, 0.10), Vector3(0.0, body_yaw, 0.0), pants_color)
    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        rotate_y(Vector3(0.14, 0.35, 0.0), body_yaw),
        Vector3(0.10, 0.35, 0.10), Vector3(0.0, body_yaw, 0.0), pants_color)

    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        rotate_y(Vector3(-0.14, 0.02, 0.0), body_yaw),
        Vector3(0.11, 0.03, 0.14), Vector3(0.0, body_yaw, 0.0), shoe_color)
    submit_player_part(world_state, camera_state.camera, world_state.sun.light,
        feet_position,
        rotate_y(Vector3(0.14, 0.02, 0.0), body_yaw),
        Vector3(0.11, 0.03, 0.14), Vector3(0.0, body_yaw, 0.0), shoe_color)
end

WorldRenderer = {
    enabled = true,

    OnLateUpdate = function(self)
        local world_state = get_world_state(self)
        local player_state = get_actor_component("player", "PlayerController")
        local camera_state = get_actor_component("camera", "CameraController")
        if world_state == nil or player_state == nil or camera_state == nil then
            return
        end

        camera_state.camera.transform.rotation_degrees = Vector3(camera_state.pitch_degrees, camera_state.yaw_degrees, 0.0)
        camera_state.camera.aspect_ratio = Application.GetScreenWidth() / Application.GetScreenHeight()

        local look_direction = camera_state:GetLookDirection()
        if camera_state.third_person_enabled then
            local desired_camera_position = Vector3.Add(
                player_state.camera_position,
                Vector3(
                    -look_direction.x * camera_state.follow_distance,
                    camera_state.follow_height - look_direction.y * camera_state.follow_distance,
                    -look_direction.z * camera_state.follow_distance
                )
            )
            camera_state.camera.transform.position = resolve_camera_position(
                world_state,
                player_state.camera_position,
                desired_camera_position
            )
        else
            camera_state.camera.transform.position = Vector3.Add(
                player_state.camera_position,
                Vector3(
                    look_direction.x * camera_state.first_person_distance,
                    camera_state.first_person_height,
                    look_direction.z * camera_state.first_person_distance
                )
            )
        end

        world_state.render_transform.rotation_degrees = Vector3(0.0, 0.0, 0.0)
        world_state.render_transform.scale = Vector3(0.5, 0.5, 0.5)

        for i = 1, #world_state.block_list do
            local block = world_state.block_list[i]
            local material = world_state:GetBlockMaterial(block)
            world_state.render_transform.position = Vector3(block.x, block.y, block.z)

            Renderer3D.SubmitMesh(
                world_state.mesh,
                world_state.render_transform,
                camera_state.camera,
                world_state.sun.light,
                material
            )
        end

        if camera_state.third_person_enabled then
            render_player(world_state, player_state, camera_state)
        end

        world_state.sun.transform.position = Vector3.Add(
            player_state.camera_position,
            Vector3.Multiply(world_state.sun.light:GetDirection(), world_state.sun.distance)
        )

        local sun_color = world_state:GetColor("sun")
        Renderer3D.SubmitMesh(
            world_state.mesh,
            world_state.sun.transform,
            camera_state.camera,
            world_state.sun.light,
            world_state.materials.sun
        )
    end
}

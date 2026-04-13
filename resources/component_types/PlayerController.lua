local function get_world_state()
    local world_actor = Actor.Find("world")
    if world_actor == nil then
        return nil
    end
    return world_actor:GetComponent("BlockWorld")
end

local function sample_ground_height_with_raycast(position, sample_radius, max_distance)
    if Physics3D == nil or Physics3D.Raycast == nil then
        return nil
    end

    local offsets = {
        Vector3(0.0, 0.0, 0.0),
        Vector3(sample_radius, 0.0, sample_radius),
        Vector3(sample_radius, 0.0, -sample_radius),
        Vector3(-sample_radius, 0.0, sample_radius),
        Vector3(-sample_radius, 0.0, -sample_radius)
    }

    local highest_y = nil
    for i = 1, #offsets do
        local origin = Vector3.Add(position, offsets[i])
        local hit = Physics3D.Raycast(origin, Vector3(0.0, -1.0, 0.0), max_distance)
        if hit ~= nil and hit.point ~= nil then
            if highest_y == nil or hit.point.y > highest_y then
                highest_y = hit.point.y
            end
        end
    end

    return highest_y
end

PlayerController = {
    enabled = true,
    move_speed = 5.5,
    eye_height = 1.6,
    player_height = 1.8,
    player_radius = 0.28,
    max_step_height = 1.05,
    collision_skin = 0.05,
    ground_follow_speed = 10.0,
    step_rise_speed = 14.0,
    gravity = 20.0,
    jump_speed = 8.0,
    ground_acceleration = 45.0,
    ground_deceleration = 38.0,
    air_acceleration = 10.0,
    air_deceleration = 1.5,
    camera_position = nil,
    movement_transform = nil,
    vertical_velocity = 0.0,
    horizontal_velocity = nil,
    visual_yaw_degrees = 0.0,
    body_yaw_degrees = 0.0,
    head_yaw_degrees = 0.0,
    head_pitch_degrees = 0.0,
    walk_cycle = 0.0,
    is_grounded = false,
    desired_ground_y = nil,

    DoesPlayerOverlapBlock = function(self, x, y, z, player_position)
        local feet_y = player_position.y - self.eye_height
        local body_top_y = feet_y + self.player_height
        local min_x = player_position.x - self.player_radius
        local max_x = player_position.x + self.player_radius
        local min_z = player_position.z - self.player_radius
        local max_z = player_position.z + self.player_radius

        local block_min_x = x - 0.5
        local block_max_x = x + 0.5
        local block_min_y = y - 0.5
        local block_max_y = y + 0.5
        local block_min_z = z - 0.5
        local block_max_z = z + 0.5

        return
            max_x > block_min_x and
            min_x < block_max_x and
            body_top_y > block_min_y + self.collision_skin and
            feet_y < block_max_y - self.collision_skin and
            max_z > block_min_z and
            min_z < block_max_z
    end,

    GetGroundHeightAt = function(self, position)
        local sample_radius = self.player_radius - self.collision_skin
        if sample_radius < 0.01 then
            sample_radius = self.player_radius
        end

        local feet_y = position.y - self.eye_height
        local ray_origin = Vector3(position.x, feet_y + self.max_step_height + 1.0, position.z)
        local hit_y = sample_ground_height_with_raycast(
            ray_origin,
            sample_radius,
            self.player_height + self.max_step_height + 3.0
        )
        if hit_y ~= nil then
            return hit_y
        end
        local world_state = get_world_state()
        if world_state ~= nil then
            return world_state:GetGroundHeightAt(position, sample_radius)
        end
        return -1000.0
    end,

    IsPlayerCollidingAt = function(self, position)
        local horizontal_half_extent = self.player_radius - self.collision_skin - 0.02
        if horizontal_half_extent < 0.08 then
            horizontal_half_extent = 0.08
        end

        local vertical_half_extent = self.player_height * 0.5 - 0.12
        if vertical_half_extent < 0.4 then
            vertical_half_extent = 0.4
        end

        local box_center = Vector3(
            position.x,
            position.y - self.eye_height + self.player_height * 0.5,
            position.z
        )
        local half_extents = Vector3(
            horizontal_half_extent,
            vertical_half_extent,
            horizontal_half_extent
        )

        if Physics3D ~= nil and Physics3D.OverlapBox ~= nil then
            return Physics3D.OverlapBox(box_center, half_extents)
        end

        return false
    end,

    TryMoveTo = function(self, target_position)
        if not self:IsPlayerCollidingAt(target_position) then
            self.camera_position = target_position
            if self.is_grounded then
                local current_feet_y = self.camera_position.y - self.eye_height
                local target_ground_height = self:GetGroundHeightAt(self.camera_position)
                local step_height = target_ground_height - current_feet_y
                if math.abs(step_height) <= self.max_step_height then
                    self.desired_ground_y = target_ground_height + self.eye_height
                end
            end
            return true
        end

        local current_feet_y = self.camera_position.y - self.eye_height
        local target_ground_height = self:GetGroundHeightAt(target_position)
        local step_height = target_ground_height - current_feet_y
        if step_height <= 0.0 or step_height > self.max_step_height then
            return false
        end

        local stepped_position = Vector3(
            target_position.x,
            target_ground_height + self.eye_height,
            target_position.z
        )
        if self:IsPlayerCollidingAt(stepped_position) then
            return false
        end

        self.camera_position = stepped_position
        self.vertical_velocity = 0.0
        self.is_grounded = true
        self.desired_ground_y = target_ground_height + self.eye_height
        return true
    end,

    MoveHorizontally = function(self, horizontal_velocity, dt)
        if Vector3.Length(horizontal_velocity) <= 0.0 then
            return horizontal_velocity
        end

        local offset = Vector3.Multiply(horizontal_velocity, dt)
        local resolved_velocity = Vector3(horizontal_velocity.x, 0.0, horizontal_velocity.z)

        local previous_x = self.camera_position.x
        local next_position_x = Vector3(self.camera_position.x + offset.x, self.camera_position.y, self.camera_position.z)
        if self:TryMoveTo(next_position_x) then
            resolved_velocity.x = (self.camera_position.x - previous_x) / dt
        else
            resolved_velocity.x = 0.0
        end

        local previous_z = self.camera_position.z
        local next_position_z = Vector3(self.camera_position.x, self.camera_position.y, self.camera_position.z + offset.z)
        if self:TryMoveTo(next_position_z) then
            resolved_velocity.z = (self.camera_position.z - previous_z) / dt
        else
            resolved_velocity.z = 0.0
        end

        return resolved_velocity
    end,

    CanPlaceBlockAt = function(self, world_state, x, y, z)
        if world_state:HasBlock(x, y, z) then
            return false
        end
        return not self:DoesPlayerOverlapBlock(x, y, z, self.camera_position)
    end,

    OnStart = function(self)
        local world_state = get_world_state()

        self.camera_position = Vector3(0.0, 1.6, 6.0)
        self.movement_transform = Transform3D()
        self.horizontal_velocity = Vector3(0.0, 0.0, 0.0)
        self.visual_yaw_degrees = 0.0
        self.body_yaw_degrees = 0.0
        self.head_yaw_degrees = 0.0
        self.head_pitch_degrees = 0.0
        self.walk_cycle = 0.0

        local ground_height = self:GetGroundHeightAt(self.camera_position)
        if (ground_height <= -999.0) and world_state ~= nil then
            ground_height = world_state:GetGroundHeightAt(self.camera_position, self.player_radius)
        end
        if ground_height > -999.0 then
            self.camera_position.y = ground_height + self.eye_height
            self.desired_ground_y = self.camera_position.y
            self.is_grounded = true
        end
    end
}

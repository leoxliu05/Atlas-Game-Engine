local function make_block_key(x, y, z)
    return tostring(x) .. ":" .. tostring(y) .. ":" .. tostring(z)
end

local function make_column_key(x, z)
    return tostring(x) .. ":" .. tostring(z)
end

local function make_height_key(x, z)
    return tostring(x) .. ":" .. tostring(z)
end

BlockWorld = {
    enabled = true,
    model_path = "resources/models/cube.obj",
    world_radius = 10,
    mesh = nil,
    render_transform = nil,
    blocks = nil,
    block_list = nil,
    column_tops = nil,
    collider_components = nil,
    colors = nil,
    materials = nil,
    sun = nil,

    GetColor = function(self, name)
        return self.colors[name]
    end,

    GetBlockMaterial = function(self, block)
        return self.materials[block.material_name]
    end,

    UpdateColumnTop = function(self, x, z)
        local column_key = make_column_key(x, z)
        local highest_y = -1000
        for i = 1, #self.block_list do
            local block = self.block_list[i]
            if block.x == x and block.z == z and block.y > highest_y then
                highest_y = block.y
            end
        end
        self.column_tops[column_key] = highest_y
    end,

    AddBlock = function(self, x, y, z, material_name)
        local key = make_block_key(x, y, z)
        if self.blocks[key] ~= nil then
            return false
        end

        local block = {
            x = x,
            y = y,
            z = z,
            material_name = material_name
        }
        self.blocks[key] = block
        table.insert(self.block_list, block)
        local rigidbody = self.actor:AddComponent("Rigidbody3D")
        if rigidbody ~= nil then
            rigidbody.position = Vector3(x, y, z)
            rigidbody.body_type = "static"
            rigidbody.collider_type = "box"
            rigidbody.width = 1.0
            rigidbody.height = 1.0
            rigidbody.depth = 1.0
            rigidbody.friction = 0.8
            rigidbody.bounciness = 0.0
            rigidbody.precise = false
            self.collider_components[key] = rigidbody
        end
        local column_key = make_column_key(x, z)
        local current_top = self.column_tops[column_key]
        if current_top == nil or y > current_top then
            self.column_tops[column_key] = y
        end
        return true
    end,

    RemoveBlock = function(self, x, y, z)
        local key = make_block_key(x, y, z)
        local block = self.blocks[key]
        if block == nil then
            return false
        end

        self.blocks[key] = nil
        local collider = self.collider_components[key]
        if collider ~= nil then
            self.actor:RemoveComponent(collider)
            self.collider_components[key] = nil
        end
        for i = #self.block_list, 1, -1 do
            local candidate = self.block_list[i]
            if candidate.x == x and candidate.y == y and candidate.z == z then
                table.remove(self.block_list, i)
                break
            end
        end
        self:UpdateColumnTop(x, z)
        return true
    end,

    HasBlock = function(self, x, y, z)
        return self.blocks[make_block_key(x, y, z)] ~= nil
    end,

    GetColumnTop = function(self, x, z)
        local value = self.column_tops[make_column_key(x, z)]
        if value == nil then
            return -1000
        end
        return value
    end,

    GetGroundHeightAt = function(self, position, radius)
        local min_x = math.floor(position.x - radius + 0.5)
        local max_x = math.floor(position.x + radius + 0.5)
        local min_z = math.floor(position.z - radius + 0.5)
        local max_z = math.floor(position.z + radius + 0.5)

        local top_y = -1000
        for x = min_x, max_x do
            for z = min_z, max_z do
                top_y = math.max(top_y, self:GetColumnTop(x, z))
            end
        end
        return top_y + 0.5
    end,

    GenerateWorld = function(self)
        local heights = {}
        for x = -self.world_radius, self.world_radius do
            for z = -self.world_radius, self.world_radius do
                local raw_height = math.floor(math.max(0.0, math.sin(x * 0.55) * 1.6 + math.cos(z * 0.45) * 1.3 + 1.4))
                heights[make_height_key(x, z)] = raw_height
            end
        end

        for pass = 1, 6 do
            for x = -self.world_radius, self.world_radius do
                for z = -self.world_radius, self.world_radius do
                    local key = make_height_key(x, z)
                    local height = heights[key]
                    local left = heights[make_height_key(x - 1, z)]
                    local right = heights[make_height_key(x + 1, z)]
                    local back = heights[make_height_key(x, z - 1)]
                    local front = heights[make_height_key(x, z + 1)]

                    if left ~= nil and height > left + 1 then
                        height = left + 1
                    end
                    if right ~= nil and height > right + 1 then
                        height = right + 1
                    end
                    if back ~= nil and height > back + 1 then
                        height = back + 1
                    end
                    if front ~= nil and height > front + 1 then
                        height = front + 1
                    end

                    heights[key] = height
                end
            end
        end

        for x = -self.world_radius, self.world_radius do
            for z = -self.world_radius, self.world_radius do
                self:AddBlock(x, -1, z, "grass")

                local hill_height = heights[make_height_key(x, z)]
                for y = 0, hill_height - 1 do
                    local material_name = "dirt"
                    if y == hill_height - 1 then
                        material_name = "grass"
                    end
                    self:AddBlock(x, y, z, material_name)
                end
            end
        end

        for y = 0, 2 do
            self:AddBlock(2, y, 2, "stone")
            self:AddBlock(3, y, 2, "stone")
            self:AddBlock(2, y, 3, "stone")
        end
    end,

    OnStart = function(self)
        self.mesh = Mesh3D()
        self.render_transform = Transform3D()
        self.blocks = {}
        self.block_list = {}
        self.column_tops = {}
        self.collider_components = {}
        self.colors = {
            skin = Vector3(0.96, 0.82, 0.68),
            shirt = Vector3(0.20, 0.52, 0.86),
            pants = Vector3(0.16, 0.22, 0.58),
            shoes = Vector3(0.18, 0.12, 0.10),
            sun = Vector3(1.0, 0.94, 0.72)
        }
        self.materials = {
            grass = Material3D(),
            dirt = Material3D(),
            stone = Material3D(),
            placed = Material3D(),
            sun = Material3D()
        }
        self.materials.grass.image_name = "materials/grass_block"
        self.materials.dirt.image_name = "materials/dirt_block"
        self.materials.stone.image_name = "materials/stone_block"
        self.materials.placed.image_name = "materials/placed_block"
        self.materials.sun.tint = self.colors.sun
        self.materials.sun.unlit = true
        self.sun = {
            transform = Transform3D(),
            light = DirectionalLight3D(),
            distance = 42.0
        }

        self.render_transform.rotation_degrees = Vector3(0.0, 0.0, 0.0)
        self.render_transform.scale = Vector3(0.5, 0.5, 0.5)
        self.sun.transform.rotation_degrees = Vector3(0.0, 0.0, 0.0)
        self.sun.transform.scale = Vector3(2.5, 2.5, 2.5)
        self.sun.light.color = Vector3(1.0, 0.96, 0.84)
        self.sun.light.intensity = 1.0
        self.sun.light:SetDirection(Vector3.Normalize(Vector3(-0.45, 0.80, -0.35)))

        if Renderer3D.SetClearColor ~= nil then
            Renderer3D.SetClearColor(0.50, 0.74, 0.98, 1.0)
        end

        if not self.mesh:LoadObj(self.model_path) then
            Debug.Log("BlockWorld: failed to load mesh at " .. self.model_path)
        end

        self:GenerateWorld()
    end
}

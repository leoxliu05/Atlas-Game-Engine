MinecraftUI = {
    enabled = true,

    -- Player stats
    health = 20,
    max_health = 20,
    hunger = 20,
    max_hunger = 20,

    -- Hotbar
    selected_slot = 0,
    hotbar_items = {"Grass", "Dirt", "Stone", "Wood", "Glass", "Brick", "Sand", "Water", "Lava"},

    -- Debug info
    show_debug = false,

    OnStart = function(self)
        -- Initialize
    end,

    OnUpdate = function(self)
        local screen_width = Application.GetScreenWidth()
        local screen_height = Application.GetScreenHeight()

        -- Toggle debug with F3
        if Input.GetKeyDown("f3") then
            self.show_debug = not self.show_debug
        end

        -- Draw hotbar slots (optimized - only draw borders and selected highlight)
        local hotbar_width = 364
        local hotbar_height = 44
        local hotbar_x = (screen_width - hotbar_width) * 0.5
        local hotbar_y = screen_height - hotbar_height - 10

        for i = 0, 8 do
            local slot_x = hotbar_x + i * 40 + 2
            local slot_y = hotbar_y + 2

            -- Only draw selected slot background
            if i == self.selected_slot then
                for y = 0, 39 do
                    for x = 0, 39 do
                        UISystem.DrawPixel(slot_x + x, slot_y + y, 255, 255, 255, 100)
                    end
                end
            end

            -- Draw slot border (4 lines instead of filling)
            for x = 0, 39 do
                UISystem.DrawPixel(slot_x + x, slot_y, 139, 139, 139, 255)
                UISystem.DrawPixel(slot_x + x, slot_y + 39, 139, 139, 139, 255)
            end
            for y = 1, 38 do
                UISystem.DrawPixel(slot_x, slot_y + y, 139, 139, 139, 255)
                UISystem.DrawPixel(slot_x + 39, slot_y + y, 139, 139, 139, 255)
            end

            -- Draw item number in slot
            UISystem.DrawText(tostring(i + 1), slot_x + 15, slot_y + 15, "Arial", 12, 255, 255, 255, 255)
        end

        -- Draw health bar (optimized - only draw filled portion and border)
        local health_bar_x = 20
        local health_bar_y = screen_height - 50
        local health_bar_width = 200
        local health_bar_height = 20

        UISystem.DrawText("Health", health_bar_x, health_bar_y - 20, "Arial", 12, 255, 255, 255, 255)

        -- Only draw the filled portion
        local health_width = math.floor((self.health / self.max_health) * health_bar_width)
        for y = 1, health_bar_height - 1 do
            for x = 1, health_width - 1 do
                UISystem.DrawPixel(health_bar_x + x, health_bar_y + y, 255, 0, 0, 255)
            end
        end

        -- Border only
        for x = 0, health_bar_width do
            UISystem.DrawPixel(health_bar_x + x, health_bar_y, 255, 255, 255, 255)
            UISystem.DrawPixel(health_bar_x + x, health_bar_y + health_bar_height, 255, 255, 255, 255)
        end
        for y = 0, health_bar_height do
            UISystem.DrawPixel(health_bar_x, health_bar_y + y, 255, 255, 255, 255)
            UISystem.DrawPixel(health_bar_x + health_bar_width, health_bar_y + y, 255, 255, 255, 255)
        end

        -- Draw hunger bar (optimized - only draw filled portion and border)
        local hunger_bar_x = screen_width - 220
        local hunger_bar_y = screen_height - 50
        local hunger_bar_width = 200
        local hunger_bar_height = 20

        UISystem.DrawText("Hunger", hunger_bar_x, hunger_bar_y - 20, "Arial", 12, 255, 255, 255, 255)

        -- Only draw the filled portion
        local hunger_width = math.floor((self.hunger / self.max_hunger) * hunger_bar_width)
        for y = 1, hunger_bar_height - 1 do
            for x = 1, hunger_width - 1 do
                UISystem.DrawPixel(hunger_bar_x + x, hunger_bar_y + y, 255, 165, 0, 255)
            end
        end

        -- Border only
        for x = 0, hunger_bar_width do
            UISystem.DrawPixel(hunger_bar_x + x, hunger_bar_y, 255, 255, 255, 255)
            UISystem.DrawPixel(hunger_bar_x + x, hunger_bar_y + hunger_bar_height, 255, 255, 255, 255)
        end
        for y = 0, hunger_bar_height do
            UISystem.DrawPixel(hunger_bar_x, hunger_bar_y + y, 255, 255, 255, 255)
            UISystem.DrawPixel(hunger_bar_x + hunger_bar_width, hunger_bar_y + y, 255, 255, 255, 255)
        end

        -- Draw crosshair (enhanced)
        local center_x = math.floor(screen_width * 0.5)
        local center_y = math.floor(screen_height * 0.5)

        -- Crosshair with outline for visibility
        for offset = -6, 6 do
            -- Black outline
            UISystem.DrawPixel(center_x + offset, center_y - 1, 0, 0, 0, 200)
            UISystem.DrawPixel(center_x + offset, center_y + 1, 0, 0, 0, 200)
            UISystem.DrawPixel(center_x - 1, center_y + offset, 0, 0, 0, 200)
            UISystem.DrawPixel(center_x + 1, center_y + offset, 0, 0, 0, 200)

            -- White crosshair
            UISystem.DrawPixel(center_x + offset, center_y, 255, 255, 255, 255)
            UISystem.DrawPixel(center_x, center_y + offset, 255, 255, 255, 255)
        end

        -- Draw debug info (F3 screen)
        if self.show_debug then
            self:DrawDebugInfo()
        end

        -- Handle hotbar selection (1-9 keys)
        for i = 1, 9 do
            if Input.GetKeyDown(tostring(i)) then
                self.selected_slot = i - 1
            end
        end

        -- Simulate health/hunger changes for demo (optional)
        -- Uncomment to see bars animate
        -- if Input.GetKeyDown("h") then
        --     self.health = math.max(0, self.health - 2)
        -- end
        -- if Input.GetKeyDown("j") then
        --     self.health = math.min(self.max_health, self.health + 2)
        -- end
    end,

    DrawDebugInfo = function(self)
        local x = 10
        local y = 10

        -- Simplified background (just border)
        for dx = 0, 300, 10 do
            for dy = 0, 200, 10 do
                UISystem.DrawPixel(x + dx, y + dy, 0, 0, 0, 150)
            end
        end

        -- Debug text
        UISystem.DrawText("Debug Screen (F3)", x + 5, y + 5, "Arial", 12, 255, 255, 0, 255)

        local player = Actor.Find("player")
        if player ~= nil then
            local player_ctrl = player:GetComponent("PlayerController")
            if player_ctrl ~= nil then
                local pos = player_ctrl.camera_position
                UISystem.DrawText("Position: " .. string.format("%.2f, %.2f, %.2f",
                    pos.x, pos.y, pos.z),
                    x + 5, y + 25, "Arial", 10, 255, 255, 255, 255)
            end
        end

        local camera = Actor.Find("camera")
        if camera ~= nil then
            local cam_ctrl = camera:GetComponent("CameraController")
            if cam_ctrl ~= nil then
                UISystem.DrawText("Yaw: " .. string.format("%.1f", cam_ctrl.yaw_degrees),
                    x + 5, y + 45, "Arial", 10, 255, 255, 255, 255)
                UISystem.DrawText("Pitch: " .. string.format("%.1f", cam_ctrl.pitch_degrees),
                    x + 5, y + 65, "Arial", 10, 255, 255, 255, 255)
            end
        end

        UISystem.DrawText("FPS: " .. string.format("%.0f", 1.0 / Application.GetDeltaTime()),
            x + 5, y + 85, "Arial", 10, 255, 255, 255, 255)
        UISystem.DrawText("Frame: " .. tostring(Application.GetFrame()),
            x + 5, y + 105, "Arial", 10, 255, 255, 255, 255)

        UISystem.DrawText("Selected Slot: " .. tostring(self.selected_slot + 1),
            x + 5, y + 125, "Arial", 10, 255, 255, 0, 255)

        UISystem.DrawText("Controls:", x + 5, y + 150, "Arial", 10, 200, 200, 200, 255)
        UISystem.DrawText("WASD - Move", x + 5, y + 165, "Arial", 9, 180, 180, 180, 255)
        UISystem.DrawText("Mouse - Look", x + 5, y + 180, "Arial", 9, 180, 180, 180, 255)
        UISystem.DrawText("1-9 - Select Item", x + 5, y + 195, "Arial", 9, 180, 180, 180, 255)
    end
}

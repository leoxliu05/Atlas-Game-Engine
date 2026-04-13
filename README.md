# Atlas Game Engine

A lightweight, Lua-scriptable 2D/3D game engine built with C++, SDL2, OpenGL, Box2D, Bullet, and GLM. The engine ships with two render paths selected at startup — a 2D SDL path and a 3D OpenGL path — and exposes a clean Lua API for gameplay scripting.

---

## Features

### Architecture
- **Actor-Component scene architecture** — scenes are JSON files; actors hold typed component instances
- **Lua gameplay scripting** — component logic is written entirely in Lua; the C++ engine handles rendering, physics, and audio
- **Event publish/subscribe system** — decoupled communication between components

### Rendering
- **2D render path (SDL)** — world-space sprite drawing, camera zoom/pan
- **3D render path (OpenGL)** — mesh submission, directional lighting, OBJ loading
- **Unified UI overlay** — screen-space 2D drawing on top of both render paths
- **Text rendering** via SDL_ttf

### Physics
- **Box2D 2D physics** — rigid bodies, triggers, collision callbacks, raycasting
- **Bullet 3D physics** — rigid bodies, raycasting, overlap queries

### Audio
- **SDL_mixer** — multi-channel audio playback, looping, volume control

### Utilities
- **Native particle system** — burst emitters with configurable color, speed, rotation, drag
- **JSON scene loading** — actors, component instances, and properties loaded from `.scene` files
- **Actor template instantiation** — spawn prefab-like actors at runtime

---

## Project Layout

```
main.cpp
  Program entry point.

engine.cpp / engine.h
  Main loop, engine startup/shutdown, and the entire Lua API binding layer.

scene_DB.cpp / scene_DB.h
  Scene loading, actor/component lifecycle, event bus.

actor.cpp / actor.h
  Actor container; owns and manages component instances.

renderer2d.cpp / renderer2d.h
  SDL-based 2D world-space renderer.

renderer3d.cpp / renderer3d.h
  OpenGL-based 3D renderer.

transform3d.cpp / transform3d.h
  3D transform math and basis vectors (right/up/forward).

camera3d.cpp / camera3d.h
  Perspective and orthographic 3D camera.

mesh3d.cpp / mesh3d.h
  Mesh data structure and OBJ loader.

material3d.h
  Material struct for 3D rendering (tint, alpha, unlit flag).

directional_light3d.h
  Directional light with transform, color, and intensity.

physics2d.cpp / physics2d.h
  Box2D integration; native Rigidbody component.

physics3d.cpp / physics3d.h
  Bullet integration; native Rigidbody3D component.

particle.h
  Native ParticleSystem component.

input.cpp / input.h
  Keyboard and mouse state polling.

resources/game.config
  Engine boot configuration (title, initial scene, render mode).

resources/rendering.config
  Resolution, clear color, zoom factor.

resources/scenes/
  Scene files (`.scene`).

resources/component_types/
  Lua component scripts. Each file defines one global table.

resources/actor_templates/
  Actor template files (`.template`).

resources/models/
  3D OBJ model files.

resources/images/
  2D image assets (PNG).

resources/fonts/
  Font files (TTF).
```

---

## Build

```bash
make
./game_engine_linux
```

Dependencies:

- `clang++`
- `SDL2`
- `SDL2_image`
- `SDL2_mixer`
- `SDL2_ttf`
- `OpenGL`
- `GLM`

---

## Configuration

The engine reads two config files at startup.

### `resources/game.config`

```json
{
  "game_title": "My Game",
  "initial_scene": "basic",
  "render_mode": "2d"
}
```

| Field | Type | Description |
|---|---|---|
| `game_title` | `string` | Window title. |
| `initial_scene` | `string` | Scene name (without `.scene` extension) to load on startup. |
| `render_mode` | `string` | `"2d"` for SDL renderer, `"3d"` for OpenGL renderer. Default: `"2d"`. |

### `resources/rendering.config`

```json
{
  "x_resolution": 1280,
  "y_resolution": 720,
  "clear_color_r": 20,
  "clear_color_g": 24,
  "clear_color_b": 32,
  "zoom_factor": 1.0
}
```

| Field | Type | Description |
|---|---|---|
| `x_resolution` | `float` | Horizontal resolution. Used by both 2D and 3D modes. |
| `y_resolution` | `float` | Vertical resolution. |
| `clear_color_r/g/b` | `int` | Clear color (0–255). |
| `zoom_factor` | `float` | 2D camera zoom. Ignored in 3D mode. |

---

## Scene Files

Scenes are JSON files under `resources/scenes/`. Each scene contains a list of actors with named component instances.

```json
{
  "actors": [
    {
      "name": "player",
      "components": {
        "1": { "type": "KeyboardMove", "speed": 5 }
      }
    }
  ]
}
```

| Field | Description |
|---|---|
| `actors` | Array of actor objects. |
| `name` | Actor display name (unique within scene). |
| `components` | Map of component instances keyed by a unique string ID. |
| `type` | Component type name. Native types: `"Rigidbody"`, `"Rigidbody3D"`, `"ParticleSystem"`. All others load from `resources/component_types/<Type>.lua`. |

---

## Lua Component Scripts

Lua component scripts live in `resources/component_types/`. The filename (without `.lua`) must match the global table defined inside.

```lua
MyComponent = {
    enabled = true,
    custom_property = 42,

    OnStart = function(self)
        -- called once when the component is first initialized
    end,

    OnUpdate = function(self)
        -- called every frame
    end,

    OnLateUpdate = function(self)
        -- called after OnUpdate, still every frame
    end,

    OnDestroy = function(self)
        -- called when the owning actor is destroyed
    end,

    OnCollisionEnter = function(self, other_actor, collision_data)
        -- called when this actor's rigid body first touches another (2D & 3D)
    end,

    OnCollisionExit = function(self, other_actor, collision_data)
        -- called when separation is detected (2D & 3D)
    end,

    OnTriggerEnter = function(self, other_actor, collision_data)
        -- called when this actor's trigger sensor is entered (2D only)
    end,

    OnTriggerExit = function(self, other_actor, collision_data)
        -- called when this actor's trigger sensor is exited (2D only)
    end,
}
```

`self` is a component instance proxy; `self.actor` is the owning actor pointer.

---

## Lua API Reference

All APIs are registered directly into the Lua global namespace. Types and namespaces are shown in PyTorch style below.

---

### Application

**`Application`**

Application-level utilities and engine introspection.

```python
Application.Quit()
```

Terminates the game loop and shuts down the engine.

```python
Application.Sleep(ms: int) -> ()
```

Suspends execution for `ms` milliseconds.

```python
Application.GetFrame() -> int
```

Returns the current frame counter.

```python
Application.GetDeltaTime() -> float
```

Returns the time elapsed since the last frame in seconds.

```python
Application.GetScreenWidth() -> float
```

Returns the horizontal resolution in pixels.

```python
Application.GetScreenHeight() -> float
```

Returns the vertical resolution in pixels.

```python
Application.OpenURL(url: str) -> ()
```

Opens a URL in the system default browser.

---

### Debug

**`Debug`**

Logging utilities.

```python
Debug.Log(message: str) -> ()
```

Prints `message` to stdout with no formatting.

---

### Input

**`Input`**

Keyboard and mouse state polling. State is reset at the end of each frame.

```python
Input.GetKey(key: str) -> bool
```

Returns `true` if `key` is currently held down. Common keys: `"w"`, `"a"`, `"s"`, `"d"`, `"space"`, `"escape"`, `"left shift"`, `"return"`.

```python
Input.GetKeyDown(key: str) -> bool
```

Returns `true` if `key` was pressed this frame (rising edge).

```python
Input.GetKeyUp(key: str) -> bool
```

Returns `true` if `key` was released this frame (falling edge).

```python
Input.GetMousePosition() -> vec2
```

Returns the current mouse position in screen pixels.

```python
Input.GetMouseDelta() -> vec2
```

Returns the mouse movement delta since the last frame in pixels.

```python
Input.GetMouseButton(button: int) -> bool
```

Returns `true` if mouse button `button` is held. Button `0` = left, `1` = right, `2` = middle.

```python
Input.GetMouseButtonDown(button: int) -> bool
```

Returns `true` if mouse button `button` was pressed this frame.

```python
Input.GetMouseButtonUp(button: int) -> bool
```

Returns `true` if mouse button `button` was released this frame.

```python
Input.GetMouseScrollDelta() -> float
```

Returns the mouse scroll delta this frame.

```python
Input.HideCursor() -> ()
```

Hides the system cursor.

```python
Input.ShowCursor() -> ()
```

Shows the system cursor.

```python
Input.SetRelativeMouseMode(enabled: bool) -> ()
```

When enabled, the cursor is hidden and movement is continuous — suitable for FPS-style camera control. Use with `Input.GetMouseDelta()`.

---

### Math Types

**`Vector3`**

A 3D floating-point vector. Constructed with `(x, y, z)` where all arguments default to `0`.

```python
v = Vector3(1, 2, 3)
v.x  -- 1.0
v.y  -- 2.0
v.z  -- 3.0
v:Length()              -> float
v:Normalize()           -> Vector3
Vector3.Dot(a, b)       -> float
Vector3.Cross(a, b)      -> Vector3
Vector3.Distance(a, b)  -> float
Vector3.Add(a, b)        -> Vector3
Vector3.Subtract(a, b)   -> Vector3
Vector3.Multiply(a, b)   -> Vector3
Vector3.MultiplyScalarLeft(s, v) -> Vector3
```

Supports operators: `+`, `-`, `*`.

**`vec2`**

A minimal 2D float vector. Constructed with `(x, y)`.

```python
v2 = vec2(1.5, 2.5)
v2.x
v2.y
vec2.Add(a, b)
vec2.Subtract(a, b)
vec2.Multiply(a, b)
vec2.Length(v)
vec2.Normalize(v)
vec2.Distance(a, b)
vec2.Dot(a, b)
```

**`Vector2`**

Box2D's 2D vector type. Constructed with `(x, y)`.

```python
v2 = Vector2(1.5, 2.5)
v2.x
v2.y
v2:Normalize() -> Vector2
v2:Length()    -> float
Vector2.Distance(a, b)
Vector2.Dot(a, b)
```

Supports operators: `+`, `-`, `*`.

---

### Actor

**`Actor`**

The base actor class. Obtain an actor instance via `Actor.Find(...)` or `Actor.Instantiate(...)`.

```python
Actor.Find(name: str) -> actor?
```

Returns the first actor in the current scene whose name matches `name`, or `nil` if none is found.

```python
Actor.FindAll(name: str) -> table
```

Returns a table of all actors in the current scene whose name matches `name`.

```python
Actor.Instantiate(template_name: str) -> actor?
```

Instantiates a new actor from a template file (`resources/actor_templates/<template_name>.template`). Returns the new actor instance, or `nil` on failure.

```python
Actor.Destroy(actor_instance: actor) -> ()
```

Destroys the actor and all its components at the end of the current frame.

**`actor` instance**

```python
:GetName() -> str
```

Returns the actor's name string.

```python
:GetID() -> int
```

Returns the actor's unique integer ID.

```python
:AddComponent(type: str) -> component_instance
```

Creates and attaches a new component of the given `type`. For native types (`"Rigidbody"`, `"Rigidbody3D"`, `"ParticleSystem"`) this creates a C++ component. For other types it loads the corresponding Lua file.

```python
:RemoveComponent(key_or_component: str or component_instance) -> ()
```

Removes the component with the given key string or instance reference.

```python
:GetComponent(type: str) -> component_instance?
```

Returns the first component of `type` on this actor, or `nil`.

```python
:GetComponents(type: str) -> table
```

Returns a table of all components of `type` on this actor.

```python
:GetComponentByKey(key: str) -> component_instance?
```

Returns the component with the given key, or `nil`.

---

### Scene

**`Scene`**

Scene management utilities.

```python
Scene.Load(scene_name: str) -> ()
```

Loads and switches to the scene named `scene_name` (without `.scene` extension) at the end of the current frame.

```python
Scene.GetCurrent() -> scene
```

Returns the current scene reference.

```python
Scene.DontDestroy(actor_instance: actor) -> ()
```

Marks the actor to survive scene transitions. Without this call, all actors are destroyed when a new scene is loaded.

---

### Event System

**`Event`**

A decoupled publish/subscribe bus. Components subscribe to named event types and receive callbacks when matching events are published.

```python
Event.Subscribe(event_type: str, component: component_instance, callback: function) -> ()
```

Registers `callback` to be invoked whenever `Event.Publish(event_type, ...)` is called. `component` is stored as the subscriber identity (used by `Unsubscribe`).

```python
Event.Unsubscribe(event_type: str, component: component_instance, callback: function) -> ()
```

Removes the exact `(component, callback)` pair from the subscriber list for `event_type`.

```python
Event.Publish(event_type: str, event_object: any) -> ()
```

Invokes all callbacks subscribed to `event_type` with `event_object` as the sole argument. Any return values from callbacks are discarded.

```lua
-- Example
function MyComponent:OnStart()
    Event.Subscribe("player_damaged", self, self.OnPlayerDamaged)
end

function MyComponent:OnPlayerDamaged(event)
    Debug.Log("Player took " .. event.damage .. " damage")
end

-- Somewhere else:
Event.Publish("player_damaged", { damage = 25 })
```

---

### 2D Physics — Box2D

#### `Rigidbody` (native component type)

A 2D physics body. Add via `actor:AddComponent("Rigidbody")` or directly in scene JSON.

**Properties (read/write):**

| Property | Type | Default | Description |
|---|---|---|---|
| `enabled` | `bool` | `true` | Whether the body participates in simulation. |
| `x`, `y` | `float` | `0` | World-space position. |
| `body_type` | `string` | `"dynamic"` | `"dynamic"`, `"static"`, or `"kinematic"`. |
| `precise` | `bool` | `false` | Enable continuous collision detection (CCD). |
| `gravity_scale` | `float` | `1.0` | Multiplier on world gravity. |
| `density` | `float` | `1.0` | Body density, used with collider size to compute mass. |
| `angular_friction` | `float` | `0.0` | Angular damping. |
| `rotation` | `float` | `0.0` | Current rotation in degrees. |
| `has_collider` | `bool` | `true` | Whether this body has a physical collider. |
| `has_trigger` | `bool` | `false` | Whether this body has a trigger sensor. |
| `collider_type` | `string` | `"box"` | `"box"` or `"circle"`. |
| `width` | `float` | `1.0` | Box collider half-width (total width = 2 * width). |
| `height` | `float` | `1.0` | Box collider half-height. |
| `radius` | `float` | `0.5` | Circle collider radius. |
| `friction` | `float` | `0.3` | Surface friction coefficient. |
| `bounciness` | `float` | `0.0` | Restitution coefficient (0 = no bounce). |
| `trigger_type` | `string` | `"box"` | Shape of the trigger sensor, independent of collider. |
| `trigger_width` | `float` | `1.0` | Trigger box half-width. |
| `trigger_height` | `float` | `1.0` | Trigger box half-height. |
| `trigger_radius` | `float` | `0.5` | Trigger circle radius. |

**Methods:**

```python
:GetPosition() -> Vector2
```

Returns the current world-space position.

```python
:GetRotation() -> float
```

Returns the current rotation in degrees.

```python
:SetPosition(x: float, y: float) -> ()
```

Teleports the body to the given world position.

```python
:SetRotation(degrees: float) -> ()
```

Sets the body rotation directly in degrees.

```python
:GetVelocity() -> Vector2
```

Returns the current linear velocity vector.

```python
:SetVelocity(x: float, y: float) -> ()
```

Sets the linear velocity directly.

```python
:GetAngularVelocity() -> float
```

Returns the angular velocity in degrees per second.

```python
:SetAngularVelocity(degrees_per_second: float) -> ()
```

Sets the angular velocity.

```python
:GetGravityScale() -> float
```

Returns the current gravity scale.

```python
:SetGravityScale(scale: float) -> ()
```

Sets the gravity scale multiplier.

```python
:AddForce(x: float, y: float) -> ()
```

Applies a central force vector in Newtons (scaled by dt internally).

```python
:GetRightDirection() -> Vector2
```

Returns the body's local right (+X) direction vector in world space.

```python
:SetRightDirection(x: float, y: float) -> ()
```

Aligns the body's local +X axis to the given world-space direction (sets rotation accordingly).

```python
:GetUpDirection() -> Vector2
```

Returns the body's local up (+Y) direction vector in world space.

```python
:SetUpDirection(x: float, y: float) -> ()
```

Aligns the body's local +Y axis to the given world-space direction.

#### `Physics` namespace

```python
Physics.Raycast(pos: Vector2, dir: Vector2, dist: float) -> RaycastHit?
```

Performs a single closest-ray cast from `pos` in direction `dir` for up to `dist` world units. Returns a `RaycastHit` table on hit, or `nil`.

**`RaycastHit` fields:**

| Field | Type | Description |
|---|---|---|
| `actor` | `actor` | The hit actor. |
| `point` | `Vector2` | World-space hit point. |
| `normal` | `Vector2` | Surface normal at the hit point. |
| `is_trigger` | `bool` | Whether the hit fixture is a trigger. |

```python
Physics.RaycastAll(pos: Vector2, dir: Vector2, dist: float) -> table
```

Like `Raycast` but returns a table of all hits sorted by distance fraction (closest first). Each entry is a `RaycastHit`.

**Collision data in callbacks (`OnCollisionEnter`, `OnCollisionExit`, `OnTriggerEnter`, `OnTriggerExit`):**

| Field | Type | Description |
|---|---|---|
| `other` | `actor` | The other actor involved in the contact. |
| `point` | `Vector2` | World-space contact point. |
| `relative_velocity` | `Vector2` | Relative velocity between the two bodies. |
| `normal` | `Vector2` | Contact normal pointing away from `self`. |

---

### 3D Physics — Bullet

#### `Rigidbody3D` (native component type)

A 3D rigid body. Add via `actor:AddComponent("Rigidbody3D")` or directly in scene JSON.

**Properties (read/write):**

| Property | Type | Default | Description |
|---|---|---|---|
| `enabled` | `bool` | `true` | Whether the body participates in simulation. |
| `position` | `Vector3` | `(0,0,0)` | World-space position. |
| `rotation_degrees` | `Vector3` | `(0,0,0)` | Euler rotation in degrees (X=pitch, Y=yaw, Z=roll). |
| `body_type` | `string` | `"dynamic"` | `"dynamic"`, `"static"`, or `"kinematic"`. |
| `collider_type` | `string` | `"box"` | `"box"` or `"sphere"`. |
| `precise` | `bool` | `false` | Enable CCD motion threshold for fast-moving objects. |
| `mass` | `float` | `1.0` | Body mass in kg. Static/kinematic bodies have mass 0. |
| `gravity_scale` | `float` | `1.0` | Multiplier on world gravity (world default = -9.8 m/s² on Y). |
| `linear_damping` | `float` | `0.0` | Linear velocity damping. |
| `angular_damping` | `float` | `0.2` | Angular velocity damping. |
| `width` | `float` | `1.0` | Box collider total width (X axis). |
| `height` | `float` | `1.0` | Box collider total height (Y axis). |
| `depth` | `float` | `1.0` | Box collider total depth (Z axis). |
| `radius` | `float` | `0.5` | Sphere collider radius. |
| `friction` | `float` | `0.5` | Surface friction coefficient. |
| `bounciness` | `float` | `0.0` | Restitution coefficient. |

**Methods:**

```python
:GetPosition() -> Vector3
```

Returns the current world-space position.

```python
:GetRotation() -> Vector3
```

Returns the current rotation as Euler angles in degrees.

```python
:SetPosition(position: Vector3) -> ()
```

Teleports the body to `position`.

```python
:SetRotation(rotation_degrees: Vector3) -> ()
```

Sets the rotation using Euler angles in degrees.

```python
:GetVelocity() -> Vector3
```

Returns the current linear velocity.

```python
:SetVelocity(velocity: Vector3) -> ()
```

Sets the linear velocity directly.

```python
:GetAngularVelocity() -> Vector3
```

Returns the current angular velocity in degrees per second.

```python
:SetAngularVelocity(degrees_per_second: Vector3) -> ()
```

Sets the angular velocity directly.

```python
:GetGravityScale() -> float
```

Returns the current gravity scale.

```python
:SetGravityScale(scale: float) -> ()
```

Sets the gravity scale multiplier.

```python
:AddForce(force: Vector3) -> ()
```

Applies a central (origin-mass) force vector in Newtons.

#### `Physics3D` namespace

```python
Physics3D.OverlapBox(position: Vector3, half_extents: Vector3) -> bool
```

Tests whether any Bullet collision object overlaps the axis-aligned box centered at `position` with half-extents `half_extents`. Returns `true` if at least one contact exists.

```python
Physics3D.Raycast(origin: Vector3, direction: Vector3, distance: float) -> RaycastHit3D?
```

Performs a closest-mesh ray cast from `origin` along `direction` for up to `distance` world units. Returns a `RaycastHit3D` table on hit, or `nil`.

**`RaycastHit3D` fields:**

| Field | Type | Description |
|---|---|---|
| `actor` | `actor` | The hit actor. |
| `point` | `Vector3` | World-space hit point. |
| `normal` | `Vector3` | Surface normal at the hit point. |
| `distance_fraction` | `float` | Fraction of `distance` at which the hit occurred (0→1). |

**Collision data in `OnCollisionEnter` / `OnCollisionExit` callbacks:**

| Field | Type | Description |
|---|---|---|
| `other` | `actor` | The other actor involved in the contact. |
| `point` | `Vector3` | World-space contact point. On `OnCollisionExit`, this is set to `(-999, -999, -999)`. |
| `relative_velocity` | `Vector3` | Relative velocity between the two bodies. |
| `normal` | `Vector3` | Contact normal pointing away from `self`. On `OnCollisionExit`, this is set to `(-999, -999, -999)`. |

> **Note:** The 3D physics system currently exposes only collision callbacks (`OnCollisionEnter`/`OnCollisionExit`). Trigger-style callbacks (`OnTriggerEnter`/`OnTriggerExit`) are not yet implemented for Rigidbody3D.

---

### 3D Rendering — OpenGL

#### `Transform3D`

Holds a 3D object's position, rotation, and scale. Constructed with no arguments.

```python
t = Transform3D()
t.position         -- Vector3, world-space position (default: 0,0,0)
t.rotation_degrees -- Vector3, Euler angles in degrees (default: 0,0,0)
t.scale            -- Vector3 (default: 1,1,1)
t:GetRightVector()   -> Vector3  -- local +X in world space
t:GetUpVector()      -> Vector3  -- local +Y in world space
t:GetForwardVector() -> Vector3  -- local +Z in world space
```

#### `Camera3D`

A 3D viewing camera. Constructed with no arguments.

```python
cam = Camera3D()
cam.transform               -- Transform3D, camera position and rotation
cam.field_of_view_degrees   -- float, vertical FOV (default: 60)
cam.aspect_ratio            -- float, width/height (set from Application.GetScreenWidth/Height)
cam.near_clip               -- float, near plane distance (default: 0.1)
cam.far_clip                -- float, far plane distance (default: 1000)
cam.is_orthographic         -- bool, use orthographic projection (default: false)
cam.orthographic_height      -- float, orthographic half-height in world units (default: 10)
```

#### `DirectionalLight3D`

A directional light source (direction determined by its transform's rotation).

```python
light = DirectionalLight3D()
light.transform   -- Transform3D, the light's orientation sets the direction
light.color       -- Vector3, RGB color (default: 1,1,1 = white)
light.intensity   -- float, light intensity multiplier (default: 1.0)
light:GetDirection()   -> Vector3  -- normalized direction the light shines (forward axis)
light:GetRadiance()    -> Vector3  -- color * intensity
light:SetDirection(direction: Vector3) -> ()  -- sets rotation so forward axis aligns with direction
```

#### `Mesh3D`

A 3D mesh container. Constructed with no arguments.

```python
mesh = Mesh3D()
mesh.name   -- string, the loaded mesh's name (empty until LoadObj is called)
mesh:Clear()                                                   -- frees mesh data
mesh:LoadObj(path: str) -> bool                                 -- loads positions and indices from an .obj file; returns true on success
mesh:HasVertices() -> bool                                     -- true if vertex data is loaded
mesh:HasIndices()  -> bool                                     -- true if index data is loaded
mesh:GetTriangleCount() -> int                                  -- number of triangles (index_count / 3), or 0 if non-indexed
```

#### `Material3D`

Defines visual properties used when submitting a mesh. Constructed with no arguments.

```python
mat = Material3D()
mat.image_name -- string, texture image name (from resources/images/), currently loaded but not yet sampled in shader
mat.tint       -- Vector3, per-vertex color multiplier (default: 1,1,1)
mat.alpha      -- float, opacity 0–1 (default: 1.0)
mat.unlit      -- bool, skip lighting and use tint as flat color (default: false)
```

Supported material names for the built-in simple shader: `"grass"`, `"dirt"`, `"stone"`, `"sun"`.

#### `Renderer3D`

Submits meshes to the OpenGL render pipeline.

```python
Renderer3D.SubmitMesh(
    mesh:      Mesh3D,
    transform: Transform3D,
    camera:    Camera3D,
    light:     DirectionalLight3D,
    material:  Material3D
) -> ()
```

Submits `mesh` with its `transform`, lit by `light`, viewed through `camera`, using `material`. All submissions are queued and flushed at the end of each render frame.

```python
Renderer3D.SetClearColor(r: float, g: float, b: float, a: float) -> ()
```

Sets the OpenGL clear color. Values are floats in [0, 1]. Only effective in 3D mode.

---

### 2D Rendering — SDL (2D mode only)

Only available when `render_mode = "2d"` in `game.config`.

#### `Image`

World-space 2D sprite drawing. Coordinates are affected by the 2D camera.

```python
Image.Draw(image_name: str, x: float, y: float) -> ()
```

Draws the image `image_name` centered at world position `(x, y)` with default scale and white tint.

```python
Image.DrawEx(
    image_name:       str,
    x:                float,
    y:                float,
    rotation_degrees: float,
    scale_x:          float,
    scale_y:          float,
    pivot_x:          float,
    pivot_y:          float,
    r:                float,
    g:                float,
    b:                float,
    a:                float,
    sorting_order:    float
) -> ()
```

Extended draw with full control. Pivot is normalized (0.5 = center). RGBA in [0, 255]. `sorting_order` is the depth layer (higher draws on top).

#### 2D Camera

```python
Camera.SetPosition(x: float, y: float) -> ()
```

Sets the 2D camera position. All `Image.Draw*` calls are world-space relative to this.

```python
Camera.GetPositionX() -> float
Camera.GetPositionY() -> float
```

Returns the current camera position.

```python
Camera.SetZoom(zoom_factor: float) -> ()
Camera.GetZoom() -> float
```

Sets/gets the camera zoom multiplier. Default: 1.0. Values > 1 zoom in; values < 1 zoom out.

---

### UI System (both 2D and 3D modes)

Screen-space 2D drawing rendered on top of everything else. Coordinates are in pixels from the top-left corner and are not affected by any camera.

```python
UISystem.DrawImage(image_name: str, x: float, y: float) -> ()
```

Draws an image at screen position `(x, y)` with default settings.

```python
UISystem.DrawImageEx(
    image_name:       str,
    x:                float,
    y:                float,
    rotation_degrees: float,
    scale_x:          float,
    scale_y:          float,
    pivot_x:          float,
    pivot_y:          float,
    r:                float,
    g:                float,
    b:                float,
    a:                float,
    sorting_order:    float
) -> ()
```

Extended UI image draw. Same semantics as `Image.DrawEx` but screen-space.

```python
UISystem.DrawText(
    content:    str,
    x:          float,
    y:          float,
    font_name:  str,
    font_size:  int,
    r:          float,
    g:          float,
    b:          float,
    a:          float
) -> ()
```

Draws text at screen position `(x, y)`. Font is loaded from `resources/fonts/<font_name>.ttf`. RGBA in [0, 255].

```python
UISystem.DrawPixel(
    x: float,
    y: float,
    r: float,
    g: float,
    b: float,
    a: float
) -> ()
```

Draws a single pixel at screen position `(x, y)`. RGBA in [0, 255].

---

### Audio

**`Audio`**

Multi-channel audio playback via SDL_mixer.

```python
Audio.Play(channel: int, clip_name: str, does_loop: bool) -> ()
```

Plays the audio file `clip_name` (without extension) on the given channel number (0–7 recommended). If `does_loop` is `true`, the clip loops indefinitely.

```python
Audio.Halt(channel: int) -> ()
```

Stops playback on `channel` immediately.

```python
Audio.SetVolume(channel: int, volume: float) -> ()
```

Sets the volume for `channel`. `volume` is in the range `[0.0, 1.0]`. Note: the `channel` parameter here is used as both a mixer channel index and a volume scalar source — pass the same channel number used in `Play`.

---

### Particle System

**`ParticleSystem`** (native component type)

A 2D burst particle emitter. Add via `actor:AddComponent("ParticleSystem")` or directly in scene JSON.

**Properties (read/write):**

| Property | Type | Default | Description |
|---|---|---|---|
| `enabled` | `bool` | `true` | Whether the system is active. |
| `x`, `y` | `float` | `0` | World-space spawn origin offset from the actor's transform. |
| `emit_angle_min` | `float` | `0` | Minimum emission angle in degrees. |
| `emit_angle_max` | `float` | `360` | Maximum emission angle in degrees. |
| `emit_radius_min` | `float` | `0` | Minimum spawn distance from origin. |
| `emit_radius_max` | `float` | `0` | Maximum spawn distance from origin. |
| `frames_between_bursts` | `int` | `60` | Frames between each automatic burst. Set to `0` to disable auto-burst. |
| `burst_quantity` | `int` | `10` | Number of particles emitted per burst. |
| `duration_frames` | `int` | `-1` | Total number of frames before the system stops. `-1` = infinite. |
| `start_scale_min` | `float` | `1.0` | Minimum initial particle scale. |
| `start_scale_max` | `float` | `1.0` | Maximum initial particle scale. |
| `start_speed_min` | `float` | `50` | Minimum initial speed. |
| `start_speed_max` | `float` | `100` | Maximum initial speed. |
| `rotation_min` | `float` | `0` | Minimum initial rotation in degrees. |
| `rotation_max` | `float` | `0` | Maximum initial rotation in degrees. |
| `rotation_speed_min` | `float` | `0` | Minimum angular velocity in degrees per frame. |
| `rotation_speed_max` | `float` | `0` | Maximum angular velocity in degrees per frame. |
| `gravity_scale_x` | `float` | `0` | Horizontal gravity applied each frame. |
| `gravity_scale_y` | `float` | `0` | Vertical gravity applied each frame. |
| `drag_factor` | `float` | `0` | Velocity multiplier applied each frame (0 = no drag, 1 = instant stop). |
| `angular_drag_factor` | `float` | `0` | Angular velocity multiplier per frame. |
| `end_scale` | `float` | `0.0` | Target scale at particle lifetime end. |
| `start_color_r/g/b/a` | `float` | varies | Initial RGBA color (0–255). |
| `end_color_r/g/b/a` | `float` | varies | Final RGBA color, lerped over lifetime. |
| `image` | `string` | `""` | Image name to use for each particle. |
| `sorting_order` | `int` | `0` | Render sorting order for particles. |

**Methods:**

```python
:Play() -> ()
```

Starts or resumes automatic burst emission.

```python
:Stop() -> ()
```

Stops the next scheduled burst. Already active particles continue to their natural end.

```python
:Burst(quantity: int) -> ()
```

Immediately emits `quantity` particles without affecting the auto-burst timer.

---

## Rendering Modes

### 2D Mode

Set `render_mode = "2d"` in `game.config`. Use this for:

- World-space 2D sprite games
- Box2D physics gameplay
- Any project where 3D rendering is not needed

Key behaviors:
- `Image.Draw*` — world-space drawing affected by the 2D camera
- `UISystem.Draw*` — screen-space UI overlay (not affected by camera)
- `Camera.SetPosition` / `Camera.SetZoom` — control the 2D camera
- `Rigidbody` and `ParticleSystem` — native 2D components

### 3D Mode

Set `render_mode = "3d"` in `game.config`. Use this for:

- OpenGL mesh rendering
- 3D physics gameplay
- Directional lighting
- HUD/UI overlay on top of a 3D scene

Key behaviors:
- `Renderer3D.SubmitMesh` — queue a mesh for the next frame
- `Mesh3D:LoadObj` — load `.obj` models from `resources/models/`
- Camera motion and yaw/pitch must be handled in Lua
- `UISystem.Draw*` — still works as screen-space overlay
- `Input.SetRelativeMouseMode(true)` — recommended for FPS-style mouse look

---

## Example Usage

### 3D mesh rendering

```lua
function RenderComponent:OnStart()
    self.mesh = Mesh3D()
    self.transform = Transform3D()
    self.camera = Camera3D()
    self.light = DirectionalLight3D()
    self.material = Material3D()

    self.mesh:LoadObj("resources/models/cube.obj")
    self.camera.aspect_ratio = Application.GetScreenWidth() / Application.GetScreenHeight()
    self.material.tint = Vector3(0.8, 0.8, 0.9)
    self.material.alpha = 1.0
end

function RenderComponent:OnUpdate()
    Renderer3D.SubmitMesh(
        self.mesh,
        self.transform,
        self.camera,
        self.light,
        self.material
    )
end
```

### 3D physics player controller

```lua
PlayerRB3D = {
    speed = 5.0,
    jump_force = 8.0,
    is_grounded = false,

    OnStart = function(self)
        self.rb = self.actor:AddComponent("Rigidbody3D")
        self.rb.body_type = "dynamic"
        self.rb.collider_type = "box"
        self.rb.mass = 1.0
    end,

    OnUpdate = function(self)
        local vx, vz = 0, 0
        if Input.GetKey("w") then vz =  1 end
        if Input.GetKey("s") then vz = -1 end
        if Input.GetKey("a") then vx = -1 end
        if Input.GetKey("d") then vx =  1 end

        local velocity = self.rb:GetVelocity()
        velocity.x = vx * self.speed
        velocity.z = vz * self.speed
        self.rb:SetVelocity(velocity)

        if Input.GetKeyDown("space") and self.is_grounded then
            self.rb:AddForce(Vector3(0, self.jump_force * self.rb.mass, 0))
        end
    end,

    OnCollisionEnter = function(self, other, data)
        if data.normal.y > 0.5 then
            self.is_grounded = true
        end
    end,

    OnCollisionExit = function(self, other, data)
        self.is_grounded = false
    end,
}
```

### 2D physics with triggers and UI

```lua
Collectible = {
    collected = false,

    OnStart = function(self)
        self.rb = self.actor:AddComponent("Rigidbody")
        self.rb.body_type = "static"
        self.rb.has_trigger = true
        self.rb.trigger_type = "circle"
        self.rb.trigger_radius = 0.5
    end,

    OnTriggerEnter = function(self, other, data)
        if self.collected then return end
        self.collected = true
        self.actor:Destroy()
        Event.Publish("coin_collected", { amount = 1 })
    end,
}

HUD = {
    coins = 0,

    OnStart = function(self)
        Event.Subscribe("coin_collected", self, self.OnCoinCollected)
    end,

    OnCoinCollected = function(self, event)
        self.coins = self.coins + event.amount
    end,

    OnLateUpdate = function(self)
        UISystem.DrawText("Coins: " .. self.coins, 24, 24, "Arial", 24, 255, 215, 0, 255)
    end,
}
```

---

## Suggested Workflow

1. Configure `game.config` and `rendering.config`
2. Decide whether your project uses `"2d"` or `"3d"` render mode
3. Create a minimal scene with one actor and one Lua component
4. Get input and a visible renderable working first
5. Add physics and gameplay logic
6. Add audio, events, and particles once the core loop is solid
7. For 3D projects, add the HUD last using `UISystem.Draw*`

---

## Current Limitations

- 2D and 3D are mutually exclusive render paths selected by `render_mode`; there is no mixed 2D-in-3D world rendering yet
- `Mesh3D:LoadObj` currently loads vertex positions and indices only; normals and UVs are not parsed
- 3D textures are loaded but not yet sampled in the shader pipeline
- 3D lighting is intentionally simple (single directional light, no shadows)
- `OnTriggerEnter`/`OnTriggerExit` are not yet wired up for `Rigidbody3D`
- Advanced features (shadows, skeletal animation, post-processing, GPU instancing, LOD) are not implemented

This engine began as a course project / teaching engine. Priorities are clarity of architecture, a simple Lua scripting workflow, and making individual engine systems easy to inspect and extend.

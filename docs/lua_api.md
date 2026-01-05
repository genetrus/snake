# Lua API (C++ ↔ Lua contract)

**Project:** `snake`  
**Runtime:** Windows, C++20 + SDL2 + Lua 5.4 (lua54.dll next to `snake.exe`)  
**Goal of this document:** define the stable interface between the C++ engine and Lua scripts: file locations, load order, hooks, data structures, error handling, and hot-reload rules.

> Status: **Draft (no-guess)** — this file includes only what is already specified and marks a few remaining design decisions as **OPEN**.  
> Those OPEN items must be resolved once, then this document becomes final.

---

## 1) Script locations

### 1.1 Repository assets
- `assets/scripts/rules.lua` — gameplay rules, formulas, spawn policy, reactions
- `assets/scripts/config.lua` — default config template copied to AppData on first run

### 1.2 User config (AppData)
- `%AppData%/snake/config.lua` — active user config (edited by the game UI)
- `%AppData%/snake/highscores.json` — highscores (JSON; not Lua)

### 1.3 Load order (engine startup)
1) Ensure `%AppData%/snake/` exists.
2) If `%AppData%/snake/config.lua` missing → copy from `assets/scripts/config.lua`.
3) Create a fresh Lua state.
4) Load **user config**: `%AppData%/snake/config.lua`.
5) Load **rules**: `assets/scripts/rules.lua`.
6) Call hook `on_init(ctx)`.

### 1.4 Hot reload (F5)
- Trigger: **F5**, works everywhere (menu / playing / paused / game over).
- Reload strategy:
  1) Create a *new* Lua state.
  2) Load `%AppData%/snake/config.lua`.
  3) Load `assets/scripts/rules.lua`.
  4) If everything succeeds → swap to the new state.
  5) If any error happens → keep the old state and show the error to the user.

---

## 2) Data model shared with Lua

Lua receives a **context table** `ctx` on every hook call.

### 2.1 Types

#### 2.1.1 Position
Grid position is an integer cell coordinate:

```lua
pos = { x = 0, y = 0 } -- 0-based indexing
```

#### 2.1.2 Direction
Direction is a string:

- `"up" | "down" | "left" | "right"`

#### 2.1.3 Bonus type
Bonus type is a string:

- `"bonus_score" | "bonus_slow"`

#### 2.1.4 Timestamp
When Lua needs timestamps (rare), it should use ISO-8601 UTC strings:
- `"2026-01-05T18:55:00Z"`

> Note: highscores timestamps are created in C++ at save time.

---

## 3) Context (`ctx`) structure

The engine guarantees that `ctx` contains at least these fields.

```lua
ctx = {
  -- immutable for the call
  phase = "menu" | "playing" | "paused" | "gameover",
  dt = number,              -- real delta time in seconds (frame delta)
  now = number,             -- monotonic time in seconds since app start

  -- configuration snapshot (loaded from %AppData%/snake/config.lua)
  config = {
    board_w = integer,      -- default 20
    board_h = integer,      -- default 20
    tile_size = integer,    -- default 32
    window_w = integer,     -- default 800
    window_h = integer,     -- default 800
    fullscreen = boolean,   -- borderless fullscreen desktop
    vsync = boolean,        -- option
    wrap_mode = boolean,    -- walls vs wrap-around

    ui_panel_mode = "auto" | "top" | "right",

    audio = {
      master = number,      -- 0..1
      sfx = number,         -- 0..1
      music = number,       -- 0..1 (reserved; music not mandatory)
    },

    -- keybinds layout is OPEN (see §10)
    keybinds = { },
  },

  -- gameplay state snapshot
  score = integer,

  board = {
    w = integer,
    h = integer,
  },

  snake = {
    dir = "up" | "down" | "left" | "right",
    length = integer,
    body = {
      -- array of positions, head first: body[1] is head
      {x=integer,y=integer},
      -- ...
    },
  },

  food = {
    pos = { x=integer, y=integer }
  },

  bonuses = {
    -- array (0..2 items)
    { type="bonus_score", pos={x=integer,y=integer} },
    { type="bonus_slow",  pos={x=integer,y=integer} },
  },

  effects = {
    slow = {
      active = boolean,
      multiplier = 0.70,     -- fixed multiplier
      remaining = number,    -- seconds remaining (0 if inactive)
    }
  },

  -- last event payload (only for event hooks; nil otherwise)
  event = nil,
}
```

---

## 4) Engine guarantees and fixed rules (from spec)

These are enforced by the C++ engine, even if Lua asks otherwise:
- Food count on board: **always exactly 1**.
- Bonus count on board: **max 2**, bonus types limited to `bonus_score`, `bonus_slow`.
- Food/bonuses never spawn on the snake.
- Snake growth:
  - Food increases length by **+1**.
  - `bonus_score` and `bonus_slow` **do not** grow the snake.
- `bonus_score` adds **+50** immediately.
- `bonus_slow`:
  - speed multiplier: **×0.70**
  - duration: **6 seconds**
  - stacking: extends time by +6 seconds, multiplier never gets stronger.
- Bonuses do **not** disappear over time (they remain until picked up).
- Collisions:
  - self-collision → **game over**
  - wall collision behavior depends on `config.wrap_mode` (walls vs wrap)

---

## 5) Hooks (C++ calls Lua)

All hooks are **optional**. If a function is missing, the engine treats it as a no-op.

### 5.1 on_init
Called once on application start (after config+rules loaded).

```lua
function on_init(ctx) end
```

### 5.2 on_tick
Called for every **fixed tick** (simulation step), not per render frame.

```lua
function on_tick(ctx) end
```

### 5.3 on_round_start
Called when a round starts or restarts.

```lua
function on_round_start(ctx) end
```

### 5.4 on_food_eaten
Called after food is eaten and score/length updates are applied.

```lua
function on_food_eaten(ctx) end
```

`ctx.event` for this hook:

```lua
ctx.event = {
  type = "food_eaten",
  food_score = integer,          -- e.g., 10
  new_score = integer,
}
```

### 5.5 on_bonus_picked
Called after a bonus is picked and its effect is applied.

```lua
function on_bonus_picked(ctx, bonus_type) end
```

`bonus_type` is `"bonus_score"` or `"bonus_slow"`.

`ctx.event` for this hook:

```lua
ctx.event = {
  type = "bonus_picked",
  bonus_type = "bonus_score" | "bonus_slow",
  new_score = integer,
  slow_remaining = number,       -- seconds (only for bonus_slow)
}
```

### 5.6 on_game_over
Called when the round ends.

```lua
function on_game_over(ctx, reason) end
```

`reason` is one of:
- `"self_collision"`
- `"wall_collision"`

### 5.7 on_setting_changed
Called when an option is changed in the UI. The engine also persists the change immediately to `%AppData%/snake/config.lua`.

```lua
function on_setting_changed(ctx, key, value) end
```

- `key`: string path, e.g. `"wrap_mode"`, `"board_w"`, `"audio.sfx"`, `"keybinds.up.primary"`, etc.
- `value`: number/string/bool/table

---

## 6) Lua-provided functions (Lua returns values to C++)

This section defines functions the engine will call to obtain formulas/policies.

> OPEN: exact function packaging (globals vs returned table) is not yet frozen; see §10.

### 6.1 Speed formula
The engine needs a function that returns the **base** speed as a function of score.

**Required behavior:**
- default base speed starts at **10 steps/sec**
- speed grows **with score**
- `bonus_slow` is applied in C++ by multiplying base speed by 0.70 while active

**OPEN (choose one):**
- Option A (recommended): return **steps_per_second**
  ```lua
  function speed_steps_per_sec(ctx, score)
    return 10.0 + score * 0.05
  end
  ```
- Option B: return **tick_dt_seconds**
  ```lua
  function tick_dt_seconds(ctx, score)
    return 1.0 / (10.0 + score * 0.05)
  end
  ```

### 6.2 Score values
Food score is fixed +10 but stored in Lua as config. The engine expects:

```lua
-- either config.food_score or rules.food_score; OPEN (see §10)
```

Bonus score is fixed +50 (engine-enforced), but can also be exposed in Lua for UI text.

### 6.3 Spawn policy for bonuses
The engine must decide **when** to spawn up to 2 bonuses (never exceeding limit).

Lua should provide a deterministic policy driven by ctx and RNG.

**OPEN (choose one):**
- Option A: engine asks after food eaten:
  ```lua
  function choose_bonus_to_spawn_after_food(ctx)
    -- return nil or "bonus_score" / "bonus_slow"
  end
  ```
- Option B: engine asks every tick:
  ```lua
  function choose_bonus_to_spawn(ctx)
    -- return nil or "bonus_score" / "bonus_slow"
  end
  ```

RNG source is OPEN (see §10).

---

## 7) Error handling (Lua side)
- Any Lua error must be converted into a readable string.
- On hot reload errors: engine keeps old Lua state and shows the error.
- On runtime hook errors (during a tick):
  - engine should treat it as non-fatal for that frame/tick, log/show error,
  - and keep running with the same Lua state.

---

## 8) Performance notes
- Hooks should be fast; avoid heavy allocations per tick.
- Prefer precomputed tables and constant lookups.
- Avoid loading files during tick; use `on_init` and hot reload.

---

## 9) Example `rules.lua` skeleton (draft)

```lua
-- rules.lua (draft skeleton)

-- OPEN: should these functions be globals or returned in a table.
-- See §10 for final decision.

function on_init(ctx)
  -- init internal state if needed
end

function on_round_start(ctx)
end

function on_tick(ctx)
end

function on_food_eaten(ctx)
end

function on_bonus_picked(ctx, bonus_type)
end

function on_game_over(ctx, reason)
end

function on_setting_changed(ctx, key, value)
end

-- OPEN: speed API
function speed_steps_per_sec(ctx, score)
  -- Example: base 10, grows slowly with score
  return 10.0 + score * 0.05
end

-- OPEN: spawn policy API
function choose_bonus_to_spawn_after_food(ctx)
  -- return nil or "bonus_score" / "bonus_slow"
  return nil
end
```

---

## 10) OPEN decisions (must be fixed once)

To finalize this contract, choose and record these decisions:

1) **Module style**: do scripts define **global functions**, or do they `return` a table?
   - Option A: globals (simple C API lookups)
   - Option B: `return { on_init=..., ... }` (namespaced)

2) **Keybind encoding in config.lua**: how keys are represented in Lua:
   - Option A: strings like `"W"`, `"Up"`, `"Esc"`
   - Option B: SDL keycodes (integers)
   - Option C: SDL scancodes (integers)

3) **Which actions are bindable**:
   - movement only, or also pause/restart/menu accept/back?

4) **Speed formula return type**:
   - steps/sec (recommended) vs tick_dt seconds

5) **Bonus spawn API**:
   - called after food eaten vs called each tick

6) **RNG ownership**:
   - engine provides `ctx.rng` (e.g., seeded int stream), or
   - Lua uses `math.random` but seeded by engine

When these are chosen, update this document and remove the OPEN markers.

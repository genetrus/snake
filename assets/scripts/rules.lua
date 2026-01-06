-- Gameplay rules implementation. All functions are global per contract.

local function cfg_get(config, path, default)
    local function traverse(tbl)
        local current = tbl
        for segment in string.gmatch(path, "([^%.]+)") do
            if type(current) ~= "table" then
                return nil
            end
            current = current[segment]
            if current == nil then
                return nil
            end
        end
        return current
    end

    local value = nil
    if config ~= nil then
        value = traverse(config)
    end
    if value == nil and _G.config ~= nil and _G.config ~= config then
        value = traverse(_G.config)
    end
    if value == nil then
        return default
    end
    return value
end

function on_app_init(ctx)
    snake.log("lua: on_app_init")
end

function on_round_start(ctx)
    snake.log("lua: on_round_start")
end

function on_round_reset(ctx)
    snake.log("lua: on_round_reset")
end

function speed_ticks_per_sec(score, config)
    local ticks = 10 + math.floor((score or 0) / 50)
    local cap = cfg_get(config, "gameplay.max_ticks_per_sec", 30)
    if cap and cap > 0 then
        ticks = math.min(ticks, cap)
    end
    return math.max(ticks, 1)
end

function resolve_wall(x, y, board_w, board_h, wrap_mode)
    if wrap_mode then
        local nx = (x % board_w + board_w) % board_w
        local ny = (y % board_h + board_h) % board_h
        return true, nx, ny
    end
    if x < 0 or x >= board_w or y < 0 or y >= board_h then
        return false, x, y
    end
    return true, x, y
end

function pickup_effect(kind, config)
    local food_score = cfg_get(config, "gameplay.food_score", 10)
    local bonus_score = cfg_get(config, "gameplay.bonus_score_score", 50)
    local slow_duration = cfg_get(config, "gameplay.slow_duration_sec", 6.0)

    if kind == "food" then
        return { score_delta = food_score, grow = true, slow_add_sec = 0 }
    elseif kind == "bonus_score" then
        return { score_delta = bonus_score, grow = false, slow_add_sec = 0 }
    elseif kind == "bonus_slow" then
        return { score_delta = 0, grow = false, slow_add_sec = slow_duration }
    end
    error("unknown pickup kind: " .. tostring(kind))
end

function want_spawn_bonus(score, config, bonuses_count)
    if bonuses_count >= 2 then
        return nil
    end

    local slow_chance = cfg_get(config, "gameplay.bonus_slow_chance", 0.05)
    local score_chance = cfg_get(config, "gameplay.bonus_score_chance", 0.08)
    local roll = math.random()

    if roll < slow_chance then
        return "bonus_slow"
    end

    if roll < slow_chance + score_chance then
        return "bonus_score"
    end

    return nil
end

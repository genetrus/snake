local rules = {
  food_score = 10, -- base points for food
  bonus_score = 50, -- base points for score bonus
  slow = {
    multiplier = 0.70, -- speed multiplier when slowed
    duration_sec = 6.0, -- duration in seconds
  },
}

-- Simple monotonic speed curve with a reasonable cap.
function rules.speed_ticks_per_sec(score)
  local base = 8 / 3
  local growth = math.floor((score or 0) / 25) / 3
  local ticks = base + growth
  return math.min(ticks, 10)
end

-- Hooks (no-op by default).
function rules.on_app_init(ctx)
end

function rules.on_tick_begin(ctx)
end

function rules.on_tick_end(ctx)
end

function rules.on_round_start(ctx)
end

function rules.on_food_eaten(ctx)
end

function rules.on_bonus_picked(ctx, type)
end

function rules.on_game_over(ctx, reason)
end

function rules.on_setting_changed(ctx, path, value)
end

return rules

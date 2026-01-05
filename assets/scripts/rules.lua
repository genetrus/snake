-- Gameplay rules stub. See docs/lua_api.md for full contract.

function on_init(ctx)
    -- initialize any Lua-side state here
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

-- Speed formula stub: base speed grows slightly with score.
function speed_steps_per_sec(ctx, score)
    return 10.0 + score * 0.05
end

-- Bonus spawn policy stub: do not spawn additional bonuses yet.
function choose_bonus_to_spawn_after_food(ctx)
    return nil
end

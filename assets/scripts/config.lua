return {
  window = {
    width = 800,
    height = 800,
    fullscreen_desktop = false,
    vsync = true,
  },

  grid = {
    board_w = 20,
    board_h = 20,
    tile_size = 32,
    wrap_mode = false,
  },

  audio = {
    enabled = true,
    master_volume = 96, -- 0..128
    sfx_volume = 96,    -- 0..128
  },

  ui = {
    panel_mode = "auto", -- "auto" | "top" | "right"
  },

  keybinds = {
    up      = { "UP", "W" },
    down    = { "DOWN", "S" },
    left    = { "LEFT", "A" },
    right   = { "RIGHT", "D" },

    pause   = { "P", "P" },
    restart = { "R", "R" },
    menu    = { "ESCAPE", "ESCAPE" },
    confirm = { "ENTER", "ENTER" },
  },

  gameplay = {
    food_score = 10,
    bonus_score_score = 50,
    slow_multiplier = 0.70,
    slow_duration_sec = 6.0,
    always_one_food = true,
    max_simultaneous_bonuses = 2,
  },
}

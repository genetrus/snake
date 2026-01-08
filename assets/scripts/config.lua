return {
  window = {
    width = 1280, -- default window width
    height = 720, -- default window height
    fullscreen_desktop = false, -- use borderless fullscreen
    vsync = true, -- enable vsync by default
  },

  grid = {
    board_w = 20, -- tiles wide
    board_h = 20, -- tiles high
    tile_size = 32, -- pixels per tile
    wrap_mode = false, -- wrap around edges
  },

  sound = {
    master = 0.8, -- master volume (0.0 - 1.0)
    sfx = 0.8, -- effects volume
    music = 0.7, -- music volume
  },

  keybinds = {
    up = { "UP", "W" },
    down = { "DOWN", "S" },
    left = { "LEFT", "A" },
    right = { "RIGHT", "D" },

    pause = { "P", "SPACE" },
    restart = { "R", "F5" },
    menu = { "ESCAPE", "F2" },
    confirm = { "ENTER", "SPACE" },
  },

  ui = {
    panel_mode = "auto", -- "auto" | "top" | "right"
  },

  gameplay = {
    food_score = 10, -- points per food
  },
}

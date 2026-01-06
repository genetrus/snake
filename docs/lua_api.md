# Lua API (контракт C++ ↔ Lua)

**Проект:** `snake`  
**Среда:** Windows, C++20 + SDL2 + Lua 5.4 (`lua54.dll` рядом с `snake.exe`)  
**Цель документа:** зафиксировать интерфейс между C++-движком и Lua-скриптами: пути, загрузку, хук-и, форматы данных, контракт правил.

> Статус: **Draft (no-guess)** — документ содержит только зафиксированные требования. Всё, что касается `rules.lua`, закреплено в разделе «Rules contract (assets/scripts/rules.lua)» и является единым источником правды.

---

## 1) Расположение скриптов и файлов

### 1.1 Репозиторий / assets
- `assets/scripts/rules.lua` — правила, формулы, политика спавна, реакции
- `assets/scripts/config.lua` — дефолтный конфиг, копируется в AppData при первом запуске

### 1.2 Пользовательские данные (AppData)
- `%AppData%/snake/config.lua` — активный конфиг (редактируется UI в игре)
- `%AppData%/snake/highscores.json` — рекорды (JSON, не Lua)

### 1.3 Порядок загрузки (старт приложения)
1) Убедиться, что `%AppData%/snake/` существует.
2) Если `%AppData%/snake/config.lua` отсутствует → копировать из `assets/scripts/config.lua`.
3) Создать новое состояние Lua.
4) Загрузить **пользовательский конфиг**: `%AppData%/snake/config.lua`.
5) Загрузить **правила**: `assets/scripts/rules.lua`.
6) Вызвать хук `on_app_init(ctx)` (если определён).

### 1.4 Хот-релоад (F5)
- Триггер: **F5** (работает в меню, во время игры, на паузе, на экране GameOver).
- Алгоритм:
  1) Создать *новое* состояние Lua.
  2) Загрузить `%AppData%/snake/config.lua`.
  3) Загрузить `assets/scripts/rules.lua`.
  4) Если всё успешно → подменить активное состояние.
  5) Если ошибка → оставить старое состояние и показать ошибку пользователю.

---

## 2) Контекст `ctx`
Движок передаёт в Lua таблицу `ctx` на каждый вызов хука.

```lua
ctx = {
  -- неизменяемые поля на время вызова
  phase = "menu" | "playing" | "paused" | "gameover",
  dt = number,              -- дельта времени рендера в секундах
  now = number,             -- монотонное время с запуска приложения, сек

  -- снапшот конфига (%AppData%/snake/config.lua)
  config = {
    board_w = integer,      -- дефолт 20
    board_h = integer,      -- дефолт 20
    tile_size = integer,    -- дефолт 32
    window_w = integer,     -- дефолт 800
    window_h = integer,     -- дефолт 800
    fullscreen = boolean,   -- borderless fullscreen desktop
    vsync = boolean,

    grid = {
      wrap_mode = boolean,  -- стенки (false) или зацикливание (true)
    },

    ui_panel_mode = "auto" | "top" | "right",

    audio = {
      master = number,      -- 0..1
      sfx = number,         -- 0..1
      music = number,       -- 0..1 (резерв)
    },

    keybinds = { },         -- схема пока открытая
  },

  -- снапшот игрового состояния
  score = integer,

  board = { w = integer, h = integer },

  snake = {
    dir = "up" | "down" | "left" | "right",
    length = integer,
    body = {
      {x=integer,y=integer}, -- массив позиций, голова = body[1]
      -- ...
    },
  },

  food = { pos = { x=integer, y=integer } },

  bonuses = {
    { type="bonus_score", pos={x=integer,y=integer} },
    { type="bonus_slow",  pos={x=integer,y=integer} },
  },

  effects = {
    slow = {
      active = boolean,
      multiplier = 0.70,     -- фиксированный множитель
      remaining = number,    -- секунды (0, если не активно)
    }
  },

  -- полезная нагрузка события (только для event-хуков; иначе nil)
  event = nil,
}
```

### 2.1 Базовые типы
- **Позиция**: `{ x = 0, y = 0 }` — целочисленные координаты клеток (0-based).
- **Направление**: строки `"up" | "down" | "left" | "right"`.
- **Тип бонуса**: строки `"bonus_score" | "bonus_slow"`.
- **Timestamp**: ISO-8601 UTC, например `"2026-01-05T18:55:00Z"` (создаются на C++ стороне при сохранении рекордов).

---

## 3) Гарантии движка (C++)
Эти правила **жёстко применяются** в C++ вне зависимости от Lua:
- Еда на поле: всегда **ровно 1**.
- Бонусы на поле: **не более 2**, типы только `bonus_score`, `bonus_slow`.
- Спавн еды/бонусов **никогда** не попадает на тело змейки.
- Рост змейки:
  - Еда: длина +1.
  - `bonus_score` и `bonus_slow` **не** растят змейку.
- `bonus_score` добавляет **+50** сразу (если Lua не переопределит сумму через контракт правил, движок всё равно применит возвращаемое значение `pickup_effect`).
- `bonus_slow`:
  - множитель скорости: **×0.70** (значение берётся из `config.gameplay.slow_multiplier`, дефолт 0.70);
  - длительность: **6 секунд** (значение из `config.gameplay.slow_duration_sec`, дефолт 6.0);
  - стак: **только продление времени**, множитель не накапливается.
- Бонусы **не исчезают** со временем — лежат, пока не взяты.
- Коллизии:
  - самоукус → **GameOver**;
  - со стеной → поведение зависит от `config.grid.wrap_mode` или логики `resolve_wall` (см. контракт правил).

---

## 4) Хуки (C++ вызывает Lua)
Все хуки задаются **глобальными функциями** в `rules.lua`. Любой отсутствующий хук трактуется как no-op.

### 4.1 Жизненный цикл и события
```lua
function on_app_init(ctx) end        -- один раз после загрузки конфигурации и правил
function on_round_start(ctx) end     -- при старте/рестарте раунда
function on_round_reset(ctx) end     -- когда раунд сброшен в исходное состояние
function on_tick(ctx) end            -- фиксированный игровой тик
function on_food_eaten(ctx) end      -- после поедания еды (score/length уже обновлены)
function on_bonus_picked(ctx, bonus_type) end  -- после поднятия бонуса
function on_game_over(ctx, reason) end         -- завершение раунда
function on_setting_changed(ctx, key, value) end -- когда настройка изменена в UI
```

`ctx.event` для некоторых хуков:

- `on_food_eaten`:
  ```lua
  ctx.event = {
    type = "food_eaten",
    food_score = integer,
    new_score = integer,
  }
  ```

- `on_bonus_picked`:
  ```lua
  ctx.event = {
    type = "bonus_picked",
    bonus_type = "bonus_score" | "bonus_slow",
    new_score = integer,
    slow_remaining = number, -- секунды (только для bonus_slow)
  }
  ```

- `on_game_over`: `reason = "self_collision" | "wall_collision"`.

Движок запускает UI/меню целиком на C++, Lua отвечает за правила, данные и реакции через эти вызовы.

---

## 5) Rules contract (`assets/scripts/rules.lua`)
Этот раздел — **единый источник правды** для контракта правил. Всё, что описано ниже, обязательно для `rules.lua` и уже подкреплено проверками на C++ стороне.

### 5.1 Общие принципы
- Скрипты правил загружаются из `./assets/scripts/` рядом с исполняемым файлом.
- Конфиг всегда читается из `%AppData%/snake/config.lua` и передаётся в Lua как `config` (см. `ctx.config`).
- Все функции правил должны быть **глобальными**, никаких возвращаемых таблиц не используется.
- Движок вызывает Lua; Lua не управляет UI или циклом игры самостоятельно.

### 5.2 Обязательные глобальные функции правил
`rules.lua` обязан определить следующие функции (их отсутствие считается ошибкой загрузки):

1. **`speed_ticks_per_sec(score, config) -> number`**
   - **Вход:** `score` (integer, текущий счёт), `config` (таблица из `%AppData%/snake/config.lua`).
   - **Выход:** `ticks_per_sec` (`number > 0`).
   - **Семантика:** задаёт базовую скорость тиков (шагов в секунду) **до** применения эффекта замедления. Движок сам умножает результат на `slow_multiplier`, когда активен эффект замедления.

2. **`resolve_wall(x, y, board_w, board_h, wrap_mode) -> (alive:boolean, nx:integer, ny:integer)`**
   - **Вход:** `x`, `y` — предполагаемая следующая клетка головы; `board_w`, `board_h` — размеры поля; `wrap_mode` — булев флаг `config.grid.wrap_mode`.
   - **Выход:**
     - `alive=false` → столкновение со стеной, наступает GameOver.
     - `alive=true` → ход разрешён, координаты скорректированы до `(nx, ny)`.
   - **Поведение по умолчанию:**
     - если `wrap_mode == true`: возвратить `alive=true` и обернуть координаты (`nx = (x % board_w + board_w) % board_w`, аналогично для `y`);
     - иначе: если выход за границы → `alive=false`; если внутри поля → `alive=true` с теми же координатами.

3. **`pickup_effect(kind, config) -> table`**
   - **Вход:** `kind` — строка `"food" | "bonus_score" | "bonus_slow"`; `config` — таблица конфига.
   - **Выход:** таблица со всеми полями (каждое поле обязательно):
     - `score_delta` (integer)
     - `grow` (boolean)
     - `slow_add_sec` (number, 0 если эффекта нет)
   - **Фиксированные правила:**
     - `food`: `score_delta = config.gameplay.food_score` (дефолт 10), `grow = true`, `slow_add_sec = 0`.
     - `bonus_score`: `score_delta = 50` (или `config.gameplay.bonus_score_score`, если поле есть), `grow = false`, `slow_add_sec = 0`.
     - `bonus_slow`: `score_delta = 0`, `grow = false`, `slow_add_sec = config.gameplay.slow_duration_sec` (дефолт 6.0).
   - **Поведение движка:** применяет `score_delta`, увеличивает длину, если `grow == true`, и продлевает таймер замедления на `slow_add_sec`, если он больше нуля. Сам множитель замедления всегда берётся из `config.gameplay.slow_multiplier` (дефолт 0.70); множители не накапливаются, удлиняется только время.

4. **`want_spawn_bonus(score, config, bonuses_count) -> (bonus_type | nil)`**
   - **Вход:** `score` (integer), `config` (таблица), `bonuses_count` (integer, текущее количество бонусов на поле).
   - **Выход:** `nil` → не спавнить бонус; `"bonus_score"` или `"bonus_slow"` → запросить спавн указанного типа.
   - **Гарантии со стороны C++:** на поле всегда **ровно 1** еда; бонусов **не более 2**; бонусы никогда не появляются на змейке; бонусы не исчезают по времени. Lua решает только политику/вероятности и тип бонуса, когда движок запрашивает спавн.

### 5.3 Дополнительные замечания
- Эффект замедления всегда задаётся движком множителем `config.gameplay.slow_multiplier` и длительностью, которая складывается по времени (`slow_add_sec`).
- Поле `wrap_mode` приходит из `config.grid.wrap_mode` (в конфиге опция хранится в `config.grid`, но в контексте `resolve_wall` передаётся отдельным аргументом для удобства).
- Все хуки из §4 и функции из этого раздела — глобальные; никаких таблиц `return` не используется.

---

## 6) Обработка ошибок (Lua)
- Любая ошибка Lua должна превращаться в читаемую строку.
- Ошибка при хот-релоаде: движок сохраняет старое состояние и показывает сообщение.
- Ошибка во время тика: движок не падает, логирует/показывает ошибку и продолжает работу с тем же состоянием Lua.

---

## 7) Замечания по производительности
- Хуки должны быть быстрыми; избегайте тяжёлых аллокаций на каждый тик.
- Предпочитайте предвычисления и константные таблицы.
- Не загружайте файлы в `on_tick`; используйте `on_app_init` и хот-релоад.

---

## 8) Пример каркаса `rules.lua`

```lua
-- rules.lua (каркас, все функции глобальные)

function on_app_init(ctx) end
function on_round_start(ctx) end
function on_round_reset(ctx) end
function on_tick(ctx) end
function on_food_eaten(ctx) end
function on_bonus_picked(ctx, bonus_type) end
function on_game_over(ctx, reason) end
function on_setting_changed(ctx, key, value) end

function speed_ticks_per_sec(score, config)
  return 10.0 + score * 0.05 -- пример
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
  if kind == "food" then
    return { score_delta = config.gameplay.food_score, grow = true, slow_add_sec = 0 }
  elseif kind == "bonus_score" then
    local bonus_score = config.gameplay.bonus_score_score or 50
    return { score_delta = bonus_score, grow = false, slow_add_sec = 0 }
  elseif kind == "bonus_slow" then
    return { score_delta = 0, grow = false, slow_add_sec = config.gameplay.slow_duration_sec }
  end
  error("unknown pickup kind: " .. tostring(kind))
end

function want_spawn_bonus(score, config, bonuses_count)
  -- пример: не спавнить, если уже есть 2 бонуса
  if bonuses_count >= 2 then return nil end
  return nil
end
```

---

## 9) Открытые вопросы (не перекрывают контракт правил)
Некоторые детали вне правил остаются на доработку и не влияют на раздел §5:
1) Кодирование `keybinds` в `config.lua` (строки/SDL keycode/SDL scancode).
2) Перечень доступных для биндинга действий (только движение или также пауза/рестарт/accept/back).
```

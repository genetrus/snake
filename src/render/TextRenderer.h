#pragma once

#include <SDL.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "render/Font.h"

namespace snake::render {

class TextRenderer {
public:
    struct Metrics {
        int w = 0;
        int h = 0;
    };

    bool Init(const std::vector<std::filesystem::path>& font_paths, int pt_size);
    void Reset();

    bool IsTtfReady() const;
    bool IsFontLoaded() const;
    const std::filesystem::path& FontPath() const;
    const std::string& LastError() const;

    Metrics MeasureText(std::string_view text, int pixel_size, bool force_bitmap = false) const;
    int DrawText(SDL_Renderer* r,
                 int x,
                 int y,
                 std::string_view text,
                 SDL_Color color,
                 int pixel_size,
                 bool force_bitmap = false);

private:
    Metrics MeasureBitmap(std::string_view text, int pixel_size) const;
    int DrawBitmap(SDL_Renderer* r, int x, int y, std::string_view text, SDL_Color color, int pixel_size) const;
    void RecordError(std::string message) const;

    Font font_;
    bool ttf_ready_ = false;
    int font_pt_size_ = 0;
    std::filesystem::path font_path_;
    mutable std::string last_error_;
};

}  // namespace snake::render

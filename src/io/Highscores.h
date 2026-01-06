#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace snake::io {

struct HighscoreEntry {
    std::string name;
    int score = 0;
    std::string achieved_at;  // ISO-8601 UTC
};

class Highscores {
public:
    bool Load(const std::filesystem::path& path);
    bool Save(const std::filesystem::path& path) const;
    const std::vector<HighscoreEntry>& Entries() const;

    // Try to insert score; keeps top-10 sorted desc by score; returns true if inserted
    bool TryAdd(std::string name, int score, std::string achieved_at_iso_utc);

    void Clear();

    static std::string NowIsoUtc();

private:
    std::vector<HighscoreEntry> entries_;
};

}  // namespace snake::io


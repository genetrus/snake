#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace snake::io {

struct Entry {
    std::string name;
    int score = 0;
    std::string achieved_at;  // ISO-8601 UTC
};

class Highscores {
public:
    bool Load(const std::filesystem::path& path);
    bool Save(const std::filesystem::path& path) const;
    const std::vector<Entry>& Entries() const;

    bool Qualifies(int score) const;

    // Try to insert score; keeps top-10 sorted desc by score; tie-breaker: earlier achieved_at first.
    bool TryInsert(const Entry& entry);

    void Clear();

    static std::string NowIsoUtc();

private:
    std::vector<Entry> entries_;
};

}  // namespace snake::io

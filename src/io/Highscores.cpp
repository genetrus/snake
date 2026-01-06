#include "io/Highscores.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>

#include "io/Config.h"
#include "nlohmann/json.hpp"

namespace snake::io {

namespace {
constexpr std::size_t kMaxEntries = 10;

bool ParseEntries(const nlohmann::json& json, std::vector<HighscoreEntry>& out) {
    std::vector<HighscoreEntry> parsed;

    if (json.is_array()) {
        for (const auto& item : json) {
            HighscoreEntry e;
            if (item.contains("name") && item["name"].is_string()) {
                e.name = item["name"].get<std::string>();
            }
            if (item.contains("score") && item["score"].is_number_integer()) {
                e.score = item["score"].get<int>();
            }
            if (item.contains("achieved_at") && item["achieved_at"].is_string()) {
                e.achieved_at = item["achieved_at"].get<std::string>();
            }
            parsed.push_back(std::move(e));
        }
    } else if (json.is_object() && json.contains("entries") && json["entries"].is_array()) {
        return ParseEntries(json["entries"], out);
    } else {
        return false;
    }

    out = std::move(parsed);
    return true;
}

}  // namespace

bool Highscores::Load(const std::filesystem::path& path) {
    entries_.clear();
    std::ifstream ifs(path);
    if (!ifs) {
        return false;
    }

    nlohmann::json json;
    try {
        ifs >> json;
    } catch (...) {
        return true;  // treat as empty
    }

    ParseEntries(json, entries_);

    // sanitize names
    for (auto& e : entries_) {
        e.name = SanitizePlayerName(std::move(e.name));
    }
    std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });
    if (entries_.size() > kMaxEntries) {
        entries_.resize(kMaxEntries);
    }
    return true;
}

bool Highscores::Save(const std::filesystem::path& path) const {
    const std::filesystem::path tmp = path.string() + ".tmp";
    std::ofstream ofs(tmp, std::ios::trunc);
    if (!ofs) {
        return false;
    }

    nlohmann::json json = nlohmann::json::array();
    for (const auto& e : entries_) {
        json.push_back(
            {{"name", e.name}, {"score", e.score}, {"achieved_at", e.achieved_at}});
    }

    ofs << json.dump(2);
    ofs.close();
    if (!ofs) {
        std::filesystem::remove(tmp);
        return false;
    }

    std::error_code ec;
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        return false;
    }
    return true;
}

const std::vector<HighscoreEntry>& Highscores::Entries() const {
    return entries_;
}

bool Highscores::TryAdd(std::string name, int score, std::string achieved_at_iso_utc) {
    name = SanitizePlayerName(std::move(name));

    HighscoreEntry entry{std::move(name), score, std::move(achieved_at_iso_utc)};
    entries_.push_back(entry);
    std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });

    bool inserted = entries_.size() <= kMaxEntries;
    if (entries_.size() > kMaxEntries) {
        inserted = false;
        entries_.resize(kMaxEntries);
        for (const auto& e : entries_) {
            if (e.name == entry.name && e.score == entry.score && e.achieved_at == entry.achieved_at) {
                inserted = true;
                break;
            }
        }
    }

    return inserted;
}

void Highscores::Clear() {
    entries_.clear();
}

std::string Highscores::NowIsoUtc() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    char buf[32]{};
    std::snprintf(
        buf,
        sizeof(buf),
        "%04d-%02d-%02dT%02d:%02d:%02dZ",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec);
    return std::string(buf);
}

}  // namespace snake::io

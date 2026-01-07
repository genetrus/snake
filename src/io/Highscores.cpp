#include "io/Highscores.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>

#include "nlohmann/json.hpp"

namespace snake::io {

namespace {
constexpr std::size_t kMaxEntries = 10;

bool ParseEntries(const nlohmann::json& json, std::vector<Entry>& out) {
    std::vector<Entry> parsed;

    if (json.is_array()) {
        for (const auto& item : json) {
            Entry e;
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

bool IsAllowedNameChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '-';
}

std::string SanitizeEntryName(std::string name) {
    name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return !IsAllowedNameChar(c); }),
               name.end());
    const std::size_t max_len = 12;
    if (name.size() > max_len) {
        name.resize(max_len);
    }
    if (name.empty()) {
        name = "Player";
    }
    return name;
}

bool EntrySortDesc(const Entry& a, const Entry& b) {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    return a.achieved_at < b.achieved_at;
}

void SortAndTrim(std::vector<Entry>& entries) {
    std::sort(entries.begin(), entries.end(), EntrySortDesc);
    if (entries.size() > kMaxEntries) {
        entries.resize(kMaxEntries);
    }
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
        e.name = SanitizeEntryName(std::move(e.name));
    }
    SortAndTrim(entries_);
    return true;
}

bool Highscores::Save(const std::filesystem::path& path) const {
    const std::filesystem::path tmp = path.string() + ".tmp";
    std::ofstream ofs(tmp, std::ios::trunc);
    if (!ofs) {
        return false;
    }

    nlohmann::json entries = nlohmann::json::array();
    for (const auto& e : entries_) {
        entries.push_back(
            {{"name", e.name}, {"score", e.score}, {"achieved_at", e.achieved_at}});
    }

    nlohmann::json json{{"version", 1}, {"entries", entries}};
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

const std::vector<Entry>& Highscores::Entries() const {
    return entries_;
}

bool Highscores::Qualifies(int score) const {
    if (entries_.size() < kMaxEntries) {
        return true;
    }
    return score > entries_.back().score;
}

bool Highscores::TryInsert(const Entry& entry) {
    Entry sanitized = entry;
    sanitized.name = SanitizeEntryName(std::move(sanitized.name));
    entries_.push_back(sanitized);
    SortAndTrim(entries_);

    return std::any_of(entries_.begin(), entries_.end(), [&](const Entry& e) {
        return e.name == sanitized.name && e.score == sanitized.score && e.achieved_at == sanitized.achieved_at;
    });
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

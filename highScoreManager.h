#pragma once
#include <string>
#include <fstream>
#include "json.hpp"

namespace HighScoreManager
{
    const std::string kFilePath = "assets/save/highscore.json";

    struct Record
    {
        int  score = 0;
        char grade = 'C';
    };

    inline std::string MakeKey(const std::wstring& songTitle, int difficultyIndex)
    {
        std::string title(songTitle.begin(), songTitle.end());
        const char* diffNames[] = { "Easy", "Normal", "Hard" };
        int idx = (difficultyIndex >= 0 && difficultyIndex <= 2) ? difficultyIndex : 1;
        return title + "_" + diffNames[idx];
    }

    inline nlohmann::json LoadAll()
    {
        std::ifstream ifs(kFilePath);
        if (!ifs.is_open())
            return nlohmann::json::object();

        nlohmann::json j;
        try { ifs >> j; }
        catch (...) { j = nlohmann::json::object(); }
        return j;
    }

    inline void SaveAll(const nlohmann::json& j)
    {
        CreateDirectoryA("assets", NULL);
        CreateDirectoryA("assets/save", NULL);

        std::ofstream ofs(kFilePath);
        if (ofs.is_open())
            ofs << j.dump(4);
    }

    inline Record GetRecord(const std::string& key)
    {
        Record rec;
        auto j = LoadAll();
        if (!j.contains(key)) return rec;

        const auto& entry = j[key];

        if (entry.is_object())
        {
            if (entry.contains("score") && entry["score"].is_number_integer())
                rec.score = entry["score"].get<int>();
            if (entry.contains("grade") && entry["grade"].is_string())
            {
                std::string g = entry["grade"].get<std::string>();
                if (!g.empty()) rec.grade = g[0];
            }
        }
        else if (entry.is_number_integer())
        {
            rec.score = entry.get<int>();
        }

        return rec;
    }

    inline int GetHighScore(const std::string& key)
    {
        return GetRecord(key).score;
    }

    inline bool TryUpdate(const std::string& key, int score, char grade = 'C')
    {
        auto j = LoadAll();
        int current = 0;
        if (j.contains(key))
        {
            const auto& entry = j[key];
            if (entry.is_object() && entry.contains("score") && entry["score"].is_number_integer())
                current = entry["score"].get<int>();
            else if (entry.is_number_integer())
                current = entry.get<int>();
        }

        if (score > current)
        {
            j[key] = { {"score", score}, {"grade", std::string(1, grade)} };
            SaveAll(j);
            return true;
        }
        return false;
    }
}
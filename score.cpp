#include "score.h"
#include <utility>
#include <fstream>
#include <string>
#include "json.hpp"

namespace
{
    const char* ToLaneKey(NoteType type)
    {
        switch (type)
        {
        case NoteType::LOW:
            return "Low";
        case NoteType::MID:
            return "Mid";
        case NoteType::HIGH:
            return "High";
        case NoteType::CRASH:
            return "Crash";
        default:
            return nullptr;
        }
    }
}

void Score::AddNote(int barNum, int beatPosition)
{
    if (barNum < 0) return;
    if (beatPosition < 0 || beatPosition >= 16) return;
    m_notes.push_back({ barNum,beatPosition });
}

void Score::AddNotes(const char* filePath)
{
    m_notes.clear();

    const char* laneKey = ToLaneKey(GetLane());
    if (!laneKey)
    {
        return;
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        return;
    }

    nlohmann::json scoreJson;
    try
    {
        file >> scoreJson;
    }
    catch (const nlohmann::json::parse_error&)
    {
        return;
    }

    if (!scoreJson.is_object())
    {
        return;
    }

    for (auto measureIt = scoreJson.begin(); measureIt != scoreJson.end(); ++measureIt)
    {
        const std::string& measureKey = measureIt.key();
        int measureIndex = 0;
        try
        {
            measureIndex = std::stoi(measureKey);
        }
        catch (const std::exception&)
        {
            continue;
        }

        if (measureIndex < 0)
        {
            continue;
        }

        const auto& measureObj = measureIt.value();
        if (!measureObj.is_object())
        {
            continue;
        }

        const auto laneIt = measureObj.find(laneKey);
        if (laneIt == measureObj.end() || !laneIt->is_array())
        {
            continue;
        }

        const auto& steps = *laneIt;
        for (size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex)
        {
            const auto& stepValue = steps[stepIndex];
            bool hasNote = false;

            if (stepValue.is_boolean())
            {
                hasNote = stepValue.get<bool>();
            }
            else if (stepValue.is_number_integer() || stepValue.is_number_unsigned())
            {
                hasNote = stepValue.get<int>() != 0;
            }

            if (hasNote)
            {
                AddNote(measureIndex, static_cast<int>(stepIndex));
            }
        }
    }
}

LowScore::LowScore(const char* filePath)
{
    AddNotes(filePath);
}

MidScore::MidScore(const char* filePath)
{
    AddNotes(filePath);
}

HighScore::HighScore(const char* filePath)
{
    AddNotes(filePath);
}

CrashScore::CrashScore(const char* filePath)
{
    AddNotes(filePath);
}

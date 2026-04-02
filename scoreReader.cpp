#include "scoreReader.h"
#include <algorithm>

namespace
{
    constexpr int kBeatsPerMeasure = 4;
    constexpr int kStepsPerBeat = 4;
    constexpr int kStepsPerMeasure = kBeatsPerMeasure * kStepsPerBeat;

    double ToAbsoluteBeat(int barNum, int stepIndex)
    {
        const int safeBar = std::max(0, barNum);
        return static_cast<double>(safeBar * kBeatsPerMeasure) +
            static_cast<double>(stepIndex) / static_cast<double>(kStepsPerBeat);
    }
}


void ScoreReader::RegisterScore(const std::shared_ptr<Score>& score)
{
    if (!score)
    {
        return;
    }
    m_scores.push_back(score);
}

void ScoreReader::Clear()
{
    m_scores.clear();
}

std::vector<UnifiedNote> ScoreReader::BuildUnifiedNotes() const
{
    std::vector<UnifiedNote> unified;
    for (const auto& score : m_scores)
    {
        if (!score)
        {
            continue;
        }

        for (const auto& note : score->GetNotes())
        {
            const double beat = ToAbsoluteBeat(note.measureIndex, note.stepIndex);
            unified.push_back({ score->GetLane(), beat });
        }
    }

    std::sort(unified.begin(), unified.end(), [](const UnifiedNote& a, const UnifiedNote& b)
    {
        if (a.beatPosition == b.beatPosition)
        {
            return static_cast<int>(a.lane) < static_cast<int>(b.lane);
        }
        return a.beatPosition < b.beatPosition;
    });

    return unified;
}

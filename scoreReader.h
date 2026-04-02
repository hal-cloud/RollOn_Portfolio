#pragma once

#include <vector>
#include <memory>
#include "score.h"


struct UnifiedNote
{
    NoteType lane;
    double beatPosition;
};

class ScoreReader
{
public:
    void RegisterScore(const std::shared_ptr<Score>& score);
    void Clear();
    std::vector<UnifiedNote> BuildUnifiedNotes() const;
private:
    std::vector<std::shared_ptr<Score>> m_scores;
};

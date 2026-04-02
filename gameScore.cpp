#include "gameScore.h"

GameScore& GameScore::Instance()
{
    static GameScore s_instance;
    return s_instance;
}

void GameScore::BeginSong(const std::wstring& songId)
{
    m_current = ScoreResult{};
    m_current.songId = songId;
    m_perfectPoint = kDefaultPerfectPoint;
    m_expectedTotalNotes = 0;
}

void GameScore::AddJudgePerfect(int point)
{
    if (point > 0) m_perfectPoint = point; 
    AddHit(point, m_current.perfect);
}

void GameScore::AddJudgeGood(int point)
{
    AddHit(point, m_current.good);
}

void GameScore::AddMiss()
{
    ++m_current.miss;
    m_current.combo = 0;
    UpdateDerivedResults(); 
}

void GameScore::CommitResult()
{
    UpdateDerivedResults();
    m_latest = m_current;
}

void GameScore::AddHit(int point, int& counter)
{
    ++counter;
    m_current.score += point;
    ++m_current.combo;
    m_current.maxCombo = std::max(m_current.maxCombo, m_current.combo);
    UpdateDerivedResults(); 
}

void GameScore::SetExpectedTotalNotes(int total)
{
    m_expectedTotalNotes = std::max(0, total);
}

void GameScore::UpdateDerivedResults()
{
    m_current.totalNotes = TotalNotes(m_current);

    const int denomNotes = (m_expectedTotalNotes > 0) ? m_expectedTotalNotes
                                                     : m_current.totalNotes;
    const int maxScore = denomNotes * m_perfectPoint;

    m_current.rate = (maxScore > 0)
        ? static_cast<double>(m_current.score) / static_cast<double>(maxScore)
        : 0.0;

    m_current.grade = ToGrade(m_current.rate);
}

int GameScore::TotalNotes(const ScoreResult& r) const
{
    return r.perfect + r.good + r.miss;
}

wchar_t GameScore::ToGrade(double rate) const
{
    if (rate >= kGradeThresholdS) return L'S';
    if (rate >= kGradeThresholdA) return L'A';
    if (rate >= kGradeThresholdB) return L'B';
    return L'C';
}
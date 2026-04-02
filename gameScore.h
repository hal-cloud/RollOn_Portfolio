#pragma once
#include <string>
#include <algorithm>

struct ScoreResult
{
    std::wstring songId;
    int   score      = 0;
    int   combo      = 0;
    int   maxCombo   = 0;
    int   perfect    = 0;
    int   good       = 0;
    int   miss       = 0;
    int   totalNotes = 0;
    double rate      = 0.0;
    wchar_t grade    = L'C';
};

class GameScore
{
public:
    static GameScore& Instance();

    void BeginSong(const std::wstring& songId);

    void AddJudgePerfect(int point);
    void AddJudgeGood(int point);
    void AddMiss();

    void CommitResult();

    const ScoreResult& GetCurrent() const { return m_current; }
    const ScoreResult& GetLatest()  const { return m_latest; }

    void SetPerfectPoint(int point) { m_perfectPoint = point; }
    void SetExpectedTotalNotes(int total);

private:
    GameScore() = default;

    void AddHit(int point, int& counter);
    void UpdateDerivedResults();
    wchar_t ToGrade(double rate) const;
    int TotalNotes(const ScoreResult& r) const;

    static constexpr int   kDefaultPerfectPoint = 1000;
    static constexpr double kGradeThresholdS    = 0.90;
    static constexpr double kGradeThresholdA    = 0.75;
    static constexpr double kGradeThresholdB    = 0.60;

    int m_perfectPoint = kDefaultPerfectPoint;
    int m_expectedTotalNotes = 0;
    ScoreResult m_current{};
    ScoreResult m_latest{};
};
#include "gameScore.h"

//=========================================================
// Instance
// static ローカル変数による遅延初期化シングルトン。
// C++11 以降はスレッドセーフが保証される。
//=========================================================
GameScore& GameScore::Instance()
{
    static GameScore s_instance;
    return s_instance;
}

//=========================================================
// BeginSong
// 曲開始時に呼び出し、集計中の結果をリセットする。
// m_perfectPoint もデフォルト値に戻し、譜面ごとの設定を引き継がないようにする。
//=========================================================
void GameScore::BeginSong(const std::wstring& songId)
{
    m_current = ScoreResult{};
    m_current.songId = songId;
    m_perfectPoint = kDefaultPerfectPoint;
    m_expectedTotalNotes = 0;
}

//=========================================================
// AddJudgePerfect
// Perfect 判定を 1 件加算する。
// point > 0 のとき m_perfectPoint を上書きする。
// これにより譜面ファイル側から動的にノーツ点数を変更できる。
//=========================================================
void GameScore::AddJudgePerfect(int point)
{
    if (point > 0) m_perfectPoint = point;
    AddHit(point, m_current.perfect);
}

//=========================================================
// AddJudgeGood
// Good 判定を 1 件加算する。
// point はそのままスコアに加算し、m_perfectPoint は変更しない。
//=========================================================
void GameScore::AddJudgeGood(int point)
{
    AddHit(point, m_current.good);
}

//=========================================================
// AddMiss
// Miss 判定を 1 件加算し、コンボを 0 にリセットする。
// スコアは変化しないが達成率の分母が増えるため rate は下がる。
//=========================================================
void GameScore::AddMiss()
{
    ++m_current.miss;
    m_current.combo = 0;
    UpdateDerivedResults();
}

//=========================================================
// CommitResult
// 曲終了時に呼び出し、最終集計結果を m_latest に確定する。
// 以降はリザルト画面で GetLatest() を参照する。
//=========================================================
void GameScore::CommitResult()
{
    UpdateDerivedResults();
    m_latest = m_current;
}

//=========================================================
// AddHit
// Perfect / Good 共通の加算処理。
// スコア加算・コンボ更新・最大コンボ更新を一括で行い、
// 最後に派生値（rate / grade）を再計算する。
//=========================================================
void GameScore::AddHit(int point, int& counter)
{
    ++counter;
    m_current.score += point;
    ++m_current.combo;
    m_current.maxCombo = std::max(m_current.maxCombo, m_current.combo);
    UpdateDerivedResults();
}

//=========================================================
// SetExpectedTotalNotes
// 達成率の分母となるノーツ総数を事前に設定する。
// 譜面読み込み時に呼び出しておくと、曲中の達成率が安定する。
// 0 以下の値は無視する（負の分母を防ぐ）。
//=========================================================
void GameScore::SetExpectedTotalNotes(int total)
{
    m_expectedTotalNotes = std::max(0, total);
}

//=========================================================
// UpdateDerivedResults
// totalNotes・rate・grade を現在の集計値から再計算する。
//
// 達成率の分母は m_expectedTotalNotes が設定されている場合はその値を使い、
// 未設定（0）の場合は実際に処理したノーツ数（totalNotes）を使う。
// maxScore が 0 のとき（ノーツが 1 件もない）は rate = 0.0 とする。
//=========================================================
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

//=========================================================
// TotalNotes
// Perfect + Good + Miss の合計を返す。
// UpdateDerivedResults と CommitResult 内で使用する。
//=========================================================
int GameScore::TotalNotes(const ScoreResult& r) const
{
    return r.perfect + r.good + r.miss;
}

//=========================================================
// ToGrade
// 達成率（0.0〜1.0）からグレード文字を返す。
// 閾値は kGradeThresholdS / A / B で定義する（gameScore.h 参照）。
//=========================================================
wchar_t GameScore::ToGrade(double rate) const
{
    if (rate >= kGradeThresholdS) return L'S';
    if (rate >= kGradeThresholdA) return L'A';
    if (rate >= kGradeThresholdB) return L'B';
    return L'C';
}
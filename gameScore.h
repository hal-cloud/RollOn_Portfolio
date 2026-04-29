//=========================================================
// gameScore.h
// ゲームスコア管理クラス
//
// 1曲分の判定結果（Perfect / Good / Miss）を集計し、
// スコア・コンボ・達成率・グレードをリアルタイムに算出するシングルトン。
// BeginSong() で初期化し、CommitResult() で m_latest に確定結果を保存する。
//=========================================================
#pragma once
#include <string>
#include <algorithm>

//-----------------------------------------------------
// ScoreResult
// 1曲分のスコアデータをまとめた POD 構造体。
// m_current（集計中）と m_latest（確定済み）の両方で使い回す。
//-----------------------------------------------------
struct ScoreResult
{
    std::wstring songId;
    int   score = 0;
    int   combo = 0;
    int   maxCombo = 0;
    int   perfect = 0;
    int   good = 0;
    int   miss = 0;
    int   totalNotes = 0;
    double rate = 0.0;  // スコア / 最大スコア（0.0〜1.0）
    wchar_t grade = L'C'; // 達成率から ToGrade() で算出する
};

class GameScore
{
public:
    //-----------------------------------------------------
    // シングルトンアクセサ
    // static ローカル変数による遅延初期化（スレッドセーフ）
    //-----------------------------------------------------
    static GameScore& Instance();

    //-----------------------------------------------------
    // 曲の開始・終了
    // BeginSong   : m_current をリセットし、songId を記録する
    // CommitResult: 最終集計を行い結果を m_latest に確定する
    //-----------------------------------------------------
    void BeginSong(const std::wstring& songId);
    void CommitResult();

    //-----------------------------------------------------
    // 判定入力
    // AddJudgePerfect: point > 0 のとき m_perfectPoint も上書きする
    // AddJudgeGood   : point はそのままスコアに加算する
    // AddMiss        : コンボをリセットしてミス数を加算する
    //-----------------------------------------------------
    void AddJudgePerfect(int point);
    void AddJudgeGood(int point);
    void AddMiss();

    //-----------------------------------------------------
    // 結果取得
    // GetCurrent: 曲中にリアルタイムで参照する（HUD 表示など）
    // GetLatest : CommitResult() 後にリザルト画面で参照する
    //-----------------------------------------------------
    const ScoreResult& GetCurrent() const { return m_current; }
    const ScoreResult& GetLatest()  const { return m_latest; }

    //-----------------------------------------------------
    // パラメータ設定
    // SetPerfectPoint       : 1ノーツあたりの Perfect 点数を変更する
    // SetExpectedTotalNotes : 達成率の分母を事前に固定する
    //                         0 のままだと実際のノーツ数が分母になる
    //-----------------------------------------------------
    void SetPerfectPoint(int point) { m_perfectPoint = point; }
    void SetExpectedTotalNotes(int total);

private:
    GameScore() = default;

    //-----------------------------------------------------
    // プライベートヘルパ
    // AddHit             : Perfect / Good 共通の加算処理
    // UpdateDerivedResults: totalNotes・rate・grade を再計算する
    // ToGrade            : 達成率からグレード文字を返す
    // TotalNotes         : perfect + good + miss の合計を返す
    //-----------------------------------------------------
    void AddHit(int point, int& counter);
    void UpdateDerivedResults();
    wchar_t ToGrade(double rate) const;
    int TotalNotes(const ScoreResult& r) const;

    //-----------------------------------------------------
    // グレード判定閾値（達成率）
    // rate >= kGradeThresholdS → S、以降 A・B・C の順で判定する
    //-----------------------------------------------------
    static constexpr int    kDefaultPerfectPoint = 1000;
    static constexpr double kGradeThresholdS = 0.90;
    static constexpr double kGradeThresholdA = 0.75;
    static constexpr double kGradeThresholdB = 0.60;

    int m_perfectPoint = kDefaultPerfectPoint;
    int m_expectedTotalNotes = 0; // 0 のとき実績ノーツ数を分母にする
    ScoreResult m_current{}; // 集計中の結果（曲中リアルタイム更新）
    ScoreResult m_latest{};  // 確定済みの結果（CommitResult() 後に有効）
};
//=========================================================
// soundLoader.cpp
// サウンドローダー実装
// SoundManager のリソース管理部分を担当するクラス
//=========================================================
#include "soundLoader.h"
#include <fstream>
#include <cstring>
#include <algorithm>

//=========================================================
// 内部構造体・ヘルパ関数
// このファイル内でのみ使用するため無名 namespace に閉じ込める
//=========================================================
namespace {

    //-----------------------------------------------------
    // WAV ファイル構造体
    //-----------------------------------------------------

    // 各チャンクの共通ヘッダ（id 4バイト + サイズ 4バイト）
    struct ChunkHeader
    {
        char     id[4];
        uint32_t size;
    };

    // ファイル先頭の RIFF ヘッダ
    // "RIFF" + ファイルサイズ + "WAVE" の12バイト固定構造
    struct RiffHeader
    {
        char     riff[4];     // "RIFF"
        uint32_t fileSize;    // ファイル全体サイズ - 8
        char     wave[4];     // "WAVE"
    };

    //-----------------------------------------------------
    // チャンク位置情報
    // ファイル内の各チャンクの位置をスキャン結果として保持する。
    // offset == 0 かどうかで判定すると先頭チャンクを誤判定するため
    // found フラグで明示的に管理する。
    //-----------------------------------------------------
    struct ChunkInfo
    {
        uint32_t offset = 0;     // データ先頭位置（チャンクヘッダ直後）
        uint32_t size   = 0;     // データサイズ
        bool     found  = false; // このチャンクが見つかったか
    };

    // ScanWavChunks の返り値。fmt と data の位置を保持する
    struct WavScanResult
    {
        ChunkInfo fmt;
        ChunkInfo data;
    };

    //-----------------------------------------------------
    // RIFFヘッダ検証
    // 成功時はファイルポインタが最初のチャンクヘッダ先頭に位置する。
    // read 失敗時（ファイルが短い・破損）を明示的に弾くため
    // read の戻り値を確認してから memcmp を行う。
    //-----------------------------------------------------
    bool ValidateWavHeader(std::ifstream& file)
    {
        RiffHeader riff{};
        if (!file.read(reinterpret_cast<char*>(&riff), sizeof(riff))) return false;
        return std::memcmp(riff.riff, "RIFF", 4) == 0
            && std::memcmp(riff.wave, "WAVE", 4) == 0;
    }

    //-----------------------------------------------------
    // WAV チャンクの全スキャン（順序不問）
    //
    // WAV 仕様ではチャンクの出現順序は保証されていない。
    // （data -> fmt の順で格納されたファイルも存在する）
    // そのため1パスで読みながら処理せず、まず全チャンクの
    // 位置・サイズを収集してから seekg で直接読みに行く2パス構造にする。
    //
    // RIFF 仕様上、チャンクサイズは偶数アライン（奇数サイズの場合は
    // パディングバイトが1バイト追加される）。
    //-----------------------------------------------------
    WavScanResult ScanWavChunks(std::ifstream& file)
    {
        WavScanResult result;

        while (file)
        {
            ChunkHeader chunk{};
            file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
            if (file.gcount() < static_cast<std::streamsize>(sizeof(chunk))) break;

            // ヘッダ直後 = このチャンクのデータ開始位置
            const uint32_t dataStart = static_cast<uint32_t>(file.tellg());

            if (std::memcmp(chunk.id, "fmt ", 4) == 0)
            {
                result.fmt = { dataStart, chunk.size, true };
            }
            else if (std::memcmp(chunk.id, "data", 4) == 0)
            {
                result.data = { dataStart, chunk.size, true };
            }
            // 未知のチャンク（LIST, IDタグ等）はスキップ

            // 奇数サイズのチャンクはパディングバイトを考慮して次へ進む
            const uint32_t aligned = chunk.size + (chunk.size & 1u);
            file.seekg(dataStart + aligned);
        }

        return result;
    }

    //-----------------------------------------------------
    // fmt チャンクを読み込んで WAVEFORMATEX を埋める
    //
    // PCM の fmt チャンクは最低 16 バイト必要（WAVEFORMAT 基本部分）。
    // 16 バイト未満は不正なフォーマットとして弾く。
    // fmt チャンクが WAVEFORMATEX（18バイト）より大きい場合でも
    // 必要な分だけ読んで残りは無視する。
    //-----------------------------------------------------
    bool ReadFmtChunk(std::ifstream& file, const ChunkInfo& info, WAVEFORMATEX& outFormat)
    {
        if (!info.found || info.size < 16) return false;

        // ScanWavChunks() 終了時に EOF/fail が立っている場合があるため、
        // 第2パスの seek/read 前に状態をクリアする。
        file.clear();
        file.seekg(info.offset);
        outFormat = {};

        const uint32_t readSize = std::min<uint32_t>(
            info.size, static_cast<uint32_t>(sizeof(WAVEFORMATEX)));

        file.read(reinterpret_cast<char*>(&outFormat), readSize);
        return static_cast<bool>(file);
    }

} // namespace


//=========================================================
// メモリ全体読み込み（SE 向け）
//=========================================================

//-----------------------------------------------------
// WAV ファイルを全てメモリに展開してキーと紐付ける
//
// 短い SE に適した方式。音声データを vector<BYTE> に保持するため
// 長い BGM に使うとメモリを圧迫する。
// 同一キーが既にロード済みの場合は上書きせずスキップする。
// （再生中データのポインタが無効になることを防ぐため）
//-----------------------------------------------------
bool SoundLoader::Load(const std::string& key, const std::wstring& filename)
{
    // 既にロード済みならスキップ（再生中データの上書き防止）
    if (m_sounds.count(key)) return true;

    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    if (!ValidateWavHeader(file)) return false;

    // 第1パス：全チャンクの位置を収集
    const WavScanResult scan = ScanWavChunks(file);

    // 第2パス：fmt チャンクを読む
    WAVEFORMATEX format{};
    if (!ReadFmtChunk(file, scan.fmt, format)) return false;
    if (!scan.data.found || scan.data.size == 0) return false;

    // 第2パス：data チャンクを全て読む
    file.seekg(scan.data.offset);
    std::vector<BYTE> audioData(scan.data.size);
    file.read(reinterpret_cast<char*>(audioData.data()), scan.data.size);
    if (!file) return false;

    SoundData data;
    data.audioData = std::move(audioData);
    data.format    = format;
    m_sounds[key]  = std::move(data);
    return true;
}

//-----------------------------------------------------
// キーからサウンドデータを取得する
// 存在しない場合は nullptr を返す（呼び出し元で確認が必要）
//-----------------------------------------------------
const SoundData* SoundLoader::Get(const std::string& key) const
{
    auto it = m_sounds.find(key);
    if (it != m_sounds.end()) return &it->second;
    return nullptr;
}


//=========================================================
// ストリーミング読み込み（BGM 向け）
//=========================================================

//-----------------------------------------------------
// WAV ファイルのヘッダ情報のみを読み込んでキーと紐付ける
//
// 音声データ本体はメモリに展開しない。
// 代わりに data チャンクの「ファイル内オフセット」と「サイズ」を記録し、
// 再生時に StreamingFileReader が都度ファイルから読み込む。
// これにより長い BGM でもメモリ使用量を抑えられる。
//
// ストリーミングはファイルパスさえ同じなら何度 Open しても
// 同じ結果になるため、重複キーはスキップして問題ない。
//-----------------------------------------------------
bool SoundLoader::LoadStreaming(const std::string& key, const std::wstring& filename)
{
    // 既にロード済みならスキップ
    if (m_streamingSounds.count(key)) return true;

    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    if (!ValidateWavHeader(file)) return false;

    // 第1パス：全チャンクの位置を収集
    const WavScanResult scan = ScanWavChunks(file);

    // 第2パス：fmt チャンクを読む（data チャンクは読まない）
    WAVEFORMATEX format{};
    if (!ReadFmtChunk(file, scan.fmt, format)) return false;
    if (!scan.data.found || scan.data.size == 0) return false;

    // data チャンクの位置情報のみ記録する（本体は読まない）
    StreamingSoundData streamData;
    streamData.filename        = filename;
    streamData.format          = format;
    streamData.dataChunkOffset = scan.data.offset; // 再生時のシーク先
    streamData.dataChunkSize   = scan.data.size;   // 総再生バイト数
    m_streamingSounds[key]     = streamData;
    return true;
}

//-----------------------------------------------------
// キーからストリーミングサウンドデータを取得する
// 存在しない場合は nullptr を返す（呼び出し元で確認が必要）
//-----------------------------------------------------
const StreamingSoundData* SoundLoader::GetStreaming(const std::string& key) const
{
    auto it = m_streamingSounds.find(key);
    if (it != m_streamingSounds.end()) return &it->second;
    return nullptr;
}


//=========================================================
// リソース管理
//=========================================================

//-----------------------------------------------------
// 全サウンドデータの解放
// シーン遷移時など、全ての音源を一括破棄する際に使用する。
// 再生中のサウンドがある場合は先に SoundManager::StopAll() を呼ぶ。
//-----------------------------------------------------
void SoundLoader::Clear()
{
    m_sounds.clear();
    m_streamingSounds.clear();
}
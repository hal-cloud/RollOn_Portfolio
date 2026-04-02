#pragma once
#include <string>
#include <map>
#include <vector>
#include <xaudio2.h>
#include <fstream>
#include <memory>

// サウンドデータ構造体（メモリ読み込み用）
struct SoundData
{
    std::vector<BYTE> audioData;   // 音声データ本体
    WAVEFORMATEX format;           // フォーマット情報
};

// ストリーミングサウンドデータ構造体
struct StreamingSoundData
{
    std::wstring filename;         // ファイルパス
    WAVEFORMATEX format;           // フォーマット情報
    uint32_t dataChunkOffset;      // データチャンクの開始位置
    uint32_t dataChunkSize;        // データチャンクのサイズ
};

class SoundLoader 
{
public:
    // サウンドファイルの読み込み（メモリ全体読み込み）
    bool Load(const std::string& key, const std::wstring& filename);

    // サウンドファイルのストリーミング用読み込み（ヘッダのみ）
    bool LoadStreaming(const std::string& key, const std::wstring& filename);

    // サウンドデータ取得（メモリ読み込み用）
    const SoundData* Get(const std::string& key) const;

    // ストリーミングサウンドデータ取得
    const StreamingSoundData* GetStreaming(const std::string& key) const;

    // 全サウンドデータの解放
    void Clear();

private:
    std::map<std::string, SoundData> m_sounds;
    std::map<std::string, StreamingSoundData> m_streamingSounds;
};
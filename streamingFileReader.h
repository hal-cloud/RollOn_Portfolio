//=========================================================
// StreamingFileReader.h
// ストリーミング再生用のファイルI/O専任クラス
// SoundPlayerの再生責務とファイル読み込み責務を分離するために使用
//=========================================================
#pragma once
#include <fstream>
#include <vector>
#include <cstdint>
#include "soundLoader.h"

class StreamingFileReader
{
public:
    // ファイルを開き、再生開始オフセットにシーク
    // startOffsetBytes: dataチャンク先頭からのオフセット（秒数指定は呼び出し元で計算済み）
    bool Open(const StreamingSoundData& streamData, uint32_t startOffsetBytes = 0);
    void Close();

    // バッファに最大 bufferSize バイト読み込む。実際に読んだバイト数を返す
    uint32_t ReadChunk(std::vector<BYTE>& buffer, uint32_t bufferSize);

    // ループ時に再生開始位置へ巻き戻す
    void SeekToStart();

    bool IsOpen()       const { return m_file.is_open(); }
    bool IsEndOfData()  const { return m_position >= m_totalBytes; }
    uint32_t GetTotalBytes()    const { return m_totalBytes; }
    uint32_t GetPosition()      const { return m_position; }

private:
    std::ifstream m_file;
    uint32_t m_dataStartOffset = 0;   // ファイル先頭からdataチャンク開始位置
    uint32_t m_startOffset     = 0;   // 再生開始オフセット（ループ巻き戻し先）
    uint32_t m_totalBytes      = 0;   // 再生対象バイト数
    uint32_t m_position        = 0;   // 読み取り済みバイト数
};
//=========================================================
// streamingFileReader.h
// ストリーミング再生用ファイルI/O専任クラス
//
// SoundPlayer の「再生」責務とファイルの「読み込み」責務を分離するために設ける。
// このクラスは XAudio2 を一切知らず、ファイルの開閉・シーク・読み取りのみを行う。
// SoundPlayer は ReadChunk() が返すバイト列をそのままバッファに投入する。
//=========================================================
#pragma once
#include <fstream>
#include <vector>
#include <cstdint>
#include "soundLoader.h"


class StreamingFileReader
{
public:
    //-----------------------------------------------------
    // ファイル操作
    //-----------------------------------------------------

    // ファイルを開き、再生開始オフセットにシークする
    // startOffsetBytes: data チャンク先頭からのバイトオフセット
    //                   秒数 → バイト変換は呼び出し元（SoundPlayer）が完了済み
    bool Open(const StreamingSoundData& streamData, uint32_t startOffsetBytes = 0);

    // ファイルを閉じ、読み取り位置をリセットする
    // Open() の冒頭でも呼ばれるため、未オープン時に呼んでも安全
    void Close();

    //-----------------------------------------------------
    // データ読み取り
    //-----------------------------------------------------

    // バッファに最大 bufferSize バイト読み込み、実際に読んだバイト数を返す
    // 終端付近では bufferSize より少ない量しか返らないことがある（正常動作）
    uint32_t ReadChunk(std::vector<BYTE>& buffer, uint32_t bufferSize);

    // 再生開始位置へ巻き戻す（ループ時に使用）
    // ifstream のエラーフラグを clear() してからシークする
    void SeekToStart();

    //-----------------------------------------------------
    // 状態取得
    //-----------------------------------------------------
    bool     IsOpen()       const { return m_file.is_open(); }
    bool     IsEndOfData()  const { return m_position >= m_totalBytes; }
    uint32_t GetTotalBytes() const { return m_totalBytes; }
    uint32_t GetPosition()   const { return m_position; }

private:
    std::ifstream m_file;
    uint32_t m_dataStartOffset = 0; // ファイル先頭から data チャンク開始位置（シーク基点）
    uint32_t m_startOffset     = 0; // 再生開始オフセット（ループ巻き戻し先）
    uint32_t m_totalBytes      = 0; // 実際に再生するバイト数（chunkSize - startOffset）
    uint32_t m_position        = 0; // 読み取り済みバイト数（IsEndOfData の判定に使用）
};
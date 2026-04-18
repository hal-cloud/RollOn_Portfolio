//=========================================================
// StreamingFileReader.cpp
// ストリーミング再生用のファイルI/O専任クラス
//
// SoundPlayer の再生責務とファイル読み込み責務を分離するために設ける。
// このクラスは XAudio2 を一切知らず、ファイルの開閉・シーク・読み取りのみを行う。
//=========================================================
#include "streamingFileReader.h"
#include <algorithm>


//=========================================================
// ファイル操作
//==========================================================

//-----------------------------------------------------
// ファイルを開き、再生開始位置にシークする
//
// startOffsetBytes は dataChャンク先頭からのオフセット。
// 秒数→バイト変換は呼び出し元（SoundPlayer::CalcStartBytes）で完了済み。
//
// chunkSize == 0 や startOffsetBytes >= chunkSize は
// 再生可能なデータが存在しないため即座に失敗とする。
//-----------------------------------------------------
bool StreamingFileReader::Open(const StreamingSoundData& streamData, uint32_t startOffsetBytes)
{
    // 前回のファイルが開いていれば閉じてからリセット
    Close();

    m_file.open(streamData.filename, std::ios::binary);
    if (!m_file) return false;

    const uint32_t chunkSize = streamData.dataChunkSize;
    if (chunkSize == 0 || startOffsetBytes >= chunkSize)
    {
        Close();
        return false;
    }

    m_dataStartOffset = streamData.dataChunkOffset; // ファイル先頭からdataチャンク開始位置
    m_startOffset     = startOffsetBytes;           // ループ巻き戻し先（再生開始オフセット）
    m_totalBytes      = chunkSize - startOffsetBytes; // 実際に再生するバイト数
    m_position        = 0;

    // 再生開始位置にシーク
    m_file.seekg(m_dataStartOffset + m_startOffset, std::ios::beg);
    if (!m_file)
    {
        Close();
        return false;
    }

    return true;
}

//-----------------------------------------------------
// ファイルを閉じ、読み取り位置をリセットする
// Open() の冒頭でも呼ばれるため、未オープン時に呼んでも安全にする
//-----------------------------------------------------
void StreamingFileReader::Close()
{
    if (m_file.is_open()) m_file.close();
    m_position = 0;
}


//=========================================================
// データ読み取り
//=========================================================

//-----------------------------------------------------
// 次のチャンクをバッファに読み込む
//
// 読み取り量は「bufferSize」と「残りバイト数」の小さい方。
// 終端を超えて読もうとした場合は残り分だけ読む。
//
// read が途中で失敗した場合（gcount == 0）はファイルを閉じて 0 を返す。
// gcount が bytesToRead より少ない場合は部分読み取りとして許容する。
// （ファイル終端付近で発生する正常ケース）
//-----------------------------------------------------
uint32_t StreamingFileReader::ReadChunk(std::vector<BYTE>& buffer, uint32_t bufferSize)
{
    if (!IsOpen() || IsEndOfData()) return 0;

    const uint32_t remaining   = m_totalBytes - m_position;
    const uint32_t bytesToRead = std::min<uint32_t>(bufferSize, remaining);

    m_file.read(reinterpret_cast<char*>(buffer.data()), bytesToRead);
    const uint32_t bytesRead = static_cast<uint32_t>(m_file.gcount());

    if (bytesRead == 0)
    {
        // read 完全失敗（ファイル破損・デバイスエラーなど）
        Close();
        return 0;
    }

    m_position += bytesRead;
    return bytesRead;
}

//-----------------------------------------------------
// 再生開始位置へ巻き戻す（ループ時に使用）
//
// ifstream はエラー状態のまま seekg しても動かないため
// clear() でエラーフラグを解除してからシークする。
// m_position を 0 に戻すことで IsEndOfData() が false になり
// 次の ReadChunk() からデータを再び読めるようになる。
//-----------------------------------------------------
void StreamingFileReader::SeekToStart()
{
    if (!IsOpen()) return;
    m_file.clear(); // seekg 前にエラーフラグを解除（終端フラグも含む）
    m_file.seekg(m_dataStartOffset + m_startOffset, std::ios::beg);
    m_position = 0;
}
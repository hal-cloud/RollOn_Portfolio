//=========================================================
// soundLoader.h
// サウンドファイルの読み込み・管理クラス
//
// WAV ファイルを解析し、SE 向けのメモリ全体読み込みと
// BGM 向けのストリーミング（ヘッダのみ読み込み）の2方式を提供する。
// 読み込んだデータは文字列キーで管理し、SoundPlayer に渡す。
//=========================================================
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <xaudio2.h>
#include <fstream>
#include <memory>


//=========================================================
// データ構造体
//=========================================================

//-----------------------------------------------------
// メモリ全体読み込み用サウンドデータ（SE 向け）
// audioData に WAV の PCM データ全体を保持する。
// 短い SE に適しているが、長い BGM に使うとメモリを圧迫する。
//-----------------------------------------------------
struct SoundData
{
	std::vector<BYTE> audioData; // 音声PCMデータ本体（全体をメモリに展開）
	WAVEFORMATEX      format;    // サンプルレート・チャンネル数などのフォーマット情報
};

//-----------------------------------------------------
// ストリーミング再生用サウンドデータ（BGM 向け）
// 音声本体はメモリに保持せず、再生時に StreamingFileReader が都度ファイルから読み込む。
// dataChunkOffset と dataChunkSize で data チャンクの位置を記録しておく。
//-----------------------------------------------------
struct StreamingSoundData
{
	std::wstring filename;        // ファイルパス（再生時のオープンに使用）
	WAVEFORMATEX format;          // フォーマット情報
	uint32_t     dataChunkOffset; // ファイル先頭から data チャンク先頭までのバイトオフセット
	uint32_t     dataChunkSize;   // data チャンクの総バイト数（再生終端の判定に使用）
};


//=========================================================
// SoundLoader クラス
//=========================================================
class SoundLoader
{
public:
	//-----------------------------------------------------
	// ロード
	// 同一キーが既に登録済みの場合は上書きせずスキップする
	// （再生中データのポインタが無効になることを防ぐため）
	//-----------------------------------------------------
	bool Load(const std::string& key, const std::wstring& filename);         // SE 用：全データをメモリに展開
	bool LoadStreaming(const std::string& key, const std::wstring& filename); // BGM 用：ヘッダ情報のみ読み込む

	//-----------------------------------------------------
	// データ取得
	// 存在しないキーを指定した場合は nullptr を返す（呼び出し元で確認が必要）
	//-----------------------------------------------------
	const SoundData*          Get(const std::string& key) const;
	const StreamingSoundData* GetStreaming(const std::string& key) const;

	//-----------------------------------------------------
	// リソース管理
	// 再生中のサウンドがある場合は先に SoundManager::StopAll() を呼ぶこと
	//-----------------------------------------------------
	void Clear();

private:
	// キーとサウンドデータを対応付けるマップ
	// unordered_map は O(1) 検索のため、毎フレーム呼ばれる Get() のコストを抑えられる
	std::unordered_map<std::string, SoundData>          m_sounds;
	std::unordered_map<std::string, StreamingSoundData> m_streamingSounds;
};
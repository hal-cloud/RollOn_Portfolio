#include "soundLoader.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace {

    struct ChunkHeader {
        char id[4];
        uint32_t size;
    };

    struct RiffHeader {
        char riff[4];
        uint32_t fileSize;
        char wave[4];
    };
}

bool SoundLoader::Load(const std::string& key, const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // RIFF�w�b�_�̓ǂݍ���
    RiffHeader riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
    if (std::memcmp(riff.riff, "RIFF", 4) != 0 || std::memcmp(riff.wave, "WAVE", 4) != 0) {
        return false;
    }

    WAVEFORMATEX format = {};
    std::vector<BYTE> audioData;

    // �`�����N�����ɒT��
    while (file) {
        ChunkHeader chunk;
        file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
        if (file.gcount() < sizeof(chunk)) break;

        if (std::memcmp(chunk.id, "fmt ", 4) == 0) {
            // �t�H�[�}�b�g�`�����N
            if (chunk.size < sizeof(WAVEFORMATEX)) {
                // �g���t�H�[�}�b�g���Ή�
                file.read(reinterpret_cast<char*>(&format), chunk.size);
            }
            else {
                file.read(reinterpret_cast<char*>(&format), sizeof(WAVEFORMATEX));
                if (chunk.size > sizeof(WAVEFORMATEX)) {
                    file.seekg(chunk.size - sizeof(WAVEFORMATEX), std::ios::cur);
                }
            }
        }
        else if (std::memcmp(chunk.id, "data", 4) == 0) {
            // �f�[�^�`�����N
            audioData.resize(chunk.size);
            file.read(reinterpret_cast<char*>(audioData.data()), chunk.size);
        }
        else {
            // ���̑��̃`�����N�̓X�L�b�v
            file.seekg(chunk.size, std::ios::cur);
        }
    }

    if (audioData.empty() || format.nChannels == 0) {
        return false;
    }

    SoundData data;
    data.audioData = std::move(audioData);
    data.format = format;
    m_sounds[key] = std::move(data);
    return true;
}

const SoundData* SoundLoader::Get(const std::string& key) const {
    auto it = m_sounds.find(key);
    if (it != m_sounds.end()) {
        return &it->second;
    }
    return nullptr;
}

void SoundLoader::Clear() {
    m_sounds.clear();
    m_streamingSounds.clear();
}

bool SoundLoader::LoadStreaming(const std::string& key, const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // RIFFヘッダの読み込み
    RiffHeader riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
    if (std::memcmp(riff.riff, "RIFF", 4) != 0 || std::memcmp(riff.wave, "WAVE", 4) != 0) {
        return false;
    }

    WAVEFORMATEX format = {};
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;

    // チャンクを順に探索
    while (file) {
        ChunkHeader chunk;
        file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk));
        if (file.gcount() < sizeof(chunk)) break;

        if (std::memcmp(chunk.id, "fmt ", 4) == 0) {
            // フォーマットチャンク - 最低限のWAVEFORMATEX構造を読み込む
            size_t minFormatSize = 16; // WAVEFORMATEX の必須フィールドのサイズ
            if (chunk.size >= minFormatSize) {
                file.read(reinterpret_cast<char*>(&format), std::min<uint32_t>(chunk.size, static_cast<uint32_t>(sizeof(WAVEFORMATEX))));                // 残りのチャンクをスキップ
                if (chunk.size > sizeof(WAVEFORMATEX)) {
                    file.seekg(chunk.size - sizeof(WAVEFORMATEX), std::ios::cur);
                }
            }
        }
        else if (std::memcmp(chunk.id, "data", 4) == 0) {
            // データチャンク（位置とサイズのみ記録）
            dataOffset = static_cast<uint32_t>(file.tellg());
            dataSize = chunk.size;
            break; // データチャンクが見つかったので終了
        }
        else {
            // その他のチャンクはスキップ
            file.seekg(chunk.size, std::ios::cur);
        }
    }

    // フォーマットの検証
    if (dataOffset == 0 || format.nChannels == 0 || format.nSamplesPerSec == 0 || 
        format.wBitsPerSample == 0 || dataSize == 0) {
        return false;
    }

    StreamingSoundData streamData;
    streamData.filename = filename;
    streamData.format = format;
    streamData.dataChunkOffset = dataOffset;
    streamData.dataChunkSize = dataSize;
    m_streamingSounds[key] = streamData;
    return true;
}

const StreamingSoundData* SoundLoader::GetStreaming(const std::string& key) const {
    auto it = m_streamingSounds.find(key);
    if (it != m_streamingSounds.end()) {
        return &it->second;
    }
    return nullptr;
}
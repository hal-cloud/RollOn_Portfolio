#pragma once

#include <vector>
#include <memory>
#include "note.h"

struct NoteEvent
{
    int measureIndex = 0;
    int stepIndex = 0;
};

class Score
{
public:
    virtual ~Score() = default;

    virtual NoteType GetLane() const = 0;
    const std::vector<NoteEvent>& GetNotes() const { return m_notes; }

protected:
    std::vector<NoteEvent> m_notes;

    void AddNotes(const char* filePath);
    void AddNote(int barNum, int beatPosition);
};

class LowScore : public Score
{
public:
    LowScore(const char* filePath);
    NoteType GetLane() const override { return NoteType::LOW; }
};

class MidScore : public Score
{
public:
    MidScore(const char* filePath);
    NoteType GetLane() const override { return NoteType::MID; }
};

class HighScore : public Score
{
public:
    HighScore(const char* filePath);
    NoteType GetLane() const override { return NoteType::HIGH; }
};

class CrashScore : public Score
{
public:
    CrashScore(const char* filePath);
    NoteType GetLane() const override { return NoteType::CRASH; }
};


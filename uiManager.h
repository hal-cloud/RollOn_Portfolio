#pragma once
#include "polygon.h"
#include "scene.h"
#include "input.h"
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

struct MenuItem
{
    std::wstring texturePath;
    std::function<void()> onSelect;
    Polygon2D* polygon = nullptr;
    float baseX = 0.0f;
    float baseY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool isSlider = false;
    float sliderValue = 0.0f;
    float sliderStep = 0.05f;
    std::function<void(float)> onValueChanged;
    Polygon2D* sliderBar = nullptr;
    Polygon2D* sliderFill = nullptr;
    int sliderBarPosX = 0;
    int sliderBarPosY = 0;

    float labelOffsetX = 0.0f;
    float barOffsetX   = 0.0f;
};

class UIPanel
{
public:
    UIPanel() = default;
    ~UIPanel() = default;

    void SetBackground(float x, float y, float w, float h, const std::wstring& texPath)
    {
        m_bgX = x; m_bgY = y; m_bgW = w; m_bgH = h;
        m_bgTexture = texPath;
    }

    void SetTitle(float x, float y, float w, float h, const std::wstring& texPath)
    {
        m_titleX = x; m_titleY = y; m_titleW = w; m_titleH = h;
        m_titleTexture = texPath;
    }

    void AddItem(float x, float y, float w, float h, const std::wstring& texPath, std::function<void()> callback)
    {
        MenuItem item;
        item.baseX = x;
        item.baseY = y;
        item.width = w;
        item.height = h;
        item.texturePath = texPath;
        item.onSelect = callback;
        m_items.push_back(item);
    }

    void AddSliderItem(float x, float y, float labelW, float labelH,
                       const std::wstring& labelTexPath,
                       float initialValue, float step,
                       std::function<void(float)> onChanged,
                       float labelOfsX = -200.0f,
                       float barOfsX   = 100.0f)
    {
        MenuItem item;
        item.baseX = x;
        item.baseY = y;
        item.width = labelW;
        item.height = labelH;
        item.texturePath = labelTexPath;
        item.isSlider = true;
        item.sliderValue = initialValue;
        item.sliderStep = step;
        item.onValueChanged = onChanged;
        item.labelOffsetX = labelOfsX;
        item.barOffsetX   = barOfsX;
        m_items.push_back(item);
    }

    void Create(Scene* scene, int priority = 100)
    {
        if (!scene || m_created) return;

        if (!m_bgTexture.empty())
        {
            m_bgPolygon = scene->AddGameObject<Polygon2D>(
                LAYER_UI,
                static_cast<int>(m_bgX), static_cast<int>(m_bgY),
                static_cast<int>(m_bgW), static_cast<int>(m_bgH),
                const_cast<wchar_t*>(m_bgTexture.c_str()));
            if (m_bgPolygon)
            {
                m_bgPolygon->SetPriority(priority);
                m_bgPolygon->SetAlpha(0.0f);
            }
        }

        if (!m_titleTexture.empty())
        {
            m_titlePolygon = scene->AddGameObject<Polygon2D>(
                LAYER_UI,
                static_cast<int>(m_titleX), static_cast<int>(m_titleY),
                static_cast<int>(m_titleW), static_cast<int>(m_titleH),
                const_cast<wchar_t*>(m_titleTexture.c_str()));
            if (m_titlePolygon)
            {
                m_titlePolygon->SetPriority(priority + 1);
                m_titlePolygon->SetAlpha(0.0f);
            }
        }

        for (auto& item : m_items)
        {
            if (item.isSlider)
            {
                int labelPx = static_cast<int>(item.baseX + item.labelOffsetX);
                int labelPy = static_cast<int>(item.baseY);
                item.polygon = scene->AddGameObject<Polygon2D>(
                    LAYER_UI,
                    labelPx, labelPy,
                    static_cast<int>(item.width), static_cast<int>(item.height),
                    const_cast<wchar_t*>(item.texturePath.c_str()));

                int barPx = static_cast<int>(item.baseX + item.barOffsetX);
                int barPy = static_cast<int>(item.baseY);
                int barW  = static_cast<int>(kSliderBarW);
                int barH  = static_cast<int>(kSliderBarH);

                item.sliderBarPosX = barPx;
                item.sliderBarPosY = barPy;

                item.sliderBar = scene->AddGameObject<Polygon2D>(
                    LAYER_UI, barPx, barPy, barW, barH,
                    const_cast<wchar_t*>(L"assets\\texture\\white.png"));
                if (item.sliderBar)
                {
                    item.sliderBar->SetPriority(priority + 3);
                    item.sliderBar->SetColor(Vector3(0.15f, 0.15f, 0.15f));
                    item.sliderBar->SetAlpha(0.0f);
                }

                item.sliderFill = scene->AddGameObject<Polygon2D>(
                    LAYER_UI, barPx, barPy, barW, barH - 6,
                    const_cast<wchar_t*>(L"assets\\texture\\white.png"));
                if (item.sliderFill)
                {
                    item.sliderFill->SetPriority(priority + 4);
                    item.sliderFill->SetColor(Vector3(0.2f, 0.6f, 1.0f));
                    item.sliderFill->SetAlpha(0.0f);
                }

                UpdateSliderVisual(item);
            }
            else
            {
                item.polygon = scene->AddGameObject<Polygon2D>(
                    LAYER_UI,
                    static_cast<int>(item.baseX), static_cast<int>(item.baseY),
                    static_cast<int>(item.width), static_cast<int>(item.height),
                    const_cast<wchar_t*>(item.texturePath.c_str()));
            }

            if (item.polygon)
            {
                item.polygon->SetPriority(priority + 2);
                item.polygon->SetAlpha(0.0f);
            }
        }

        m_created = true;
    }

    void SetVisible(bool visible)
    {
        m_visible = visible;
        float bgGamma = visible ? m_bgAlpha : 0.0f;

        if (m_bgPolygon) m_bgPolygon->SetAlpha(bgGamma);
        if (m_titlePolygon) m_titlePolygon->SetAlpha(visible ? 1.0f : 0.0f);

        for (size_t i = 0; i < m_items.size(); ++i)
        {
            float itemGamma = visible ? (i == m_selectedIndex ? 1.0f : m_unselectedAlpha) : 0.0f;
            if (m_items[i].polygon) m_items[i].polygon->SetAlpha(itemGamma);
            if (m_items[i].sliderBar) m_items[i].sliderBar->SetAlpha(visible ? 0.8f : 0.0f);
            if (m_items[i].sliderFill) m_items[i].sliderFill->SetAlpha(itemGamma);
        }
    }

    void Update()
    {
        if (!m_visible || m_items.empty()) return;

        if (Input::GetKeyTrigger('W') || Input::GetKeyTrigger(VK_UP))
        {
            m_selectedIndex = (m_selectedIndex + m_items.size() - 1) % m_items.size();
            UpdateItemHighlight();
        }
        if (Input::GetKeyTrigger('S') || Input::GetKeyTrigger(VK_DOWN))
        {
            m_selectedIndex = (m_selectedIndex + 1) % m_items.size();
            UpdateItemHighlight();
        }

        auto& current = m_items[m_selectedIndex];

        if (current.isSlider)
        {
            bool changed = false;
            if (Input::GetKeyTrigger('A') || Input::GetKeyTrigger(VK_LEFT))
            {
                current.sliderValue = std::max(0.0f, current.sliderValue - current.sliderStep);
                changed = true;
            }
            if (Input::GetKeyTrigger('D') || Input::GetKeyTrigger(VK_RIGHT))
            {
                current.sliderValue = std::min(1.0f, current.sliderValue + current.sliderStep);
                changed = true;
            }
            if (changed)
            {
                UpdateSliderVisual(current);
                if (current.onValueChanged) current.onValueChanged(current.sliderValue);
            }
        }
        else
        {
            if (Input::GetKeyTrigger(VK_RETURN) || Input::GetKeyTrigger('Z'))
            {
                if (current.onSelect) current.onSelect();
            }
        }
    }

    void ResetSelection() { m_selectedIndex = 0; UpdateItemHighlight(); }

    bool IsVisible() const { return m_visible; }
    size_t GetSelectedIndex() const { return m_selectedIndex; }

    void SetBackgroundAlpha(float alpha) { m_bgAlpha = alpha; }
    void SetUnselectedAlpha(float alpha) { m_unselectedAlpha = alpha; }

private:
    static constexpr float kSliderBarW = 300.0f;
    static constexpr float kSliderBarH = 30.0f;

    void UpdateSliderVisual(MenuItem& item)
    {
        if (!item.isSlider || !item.sliderFill) return;

        float val = std::max(0.01f, item.sliderValue);
        float barCX = static_cast<float>(item.sliderBarPosX);
        float barCY = static_cast<float>(item.sliderBarPosY);

        float fillCX = barCX - kSliderBarW * (1.0f - val) * 0.5f;

        item.sliderFill->SetPosition(Vector3(fillCX, barCY, 0.0f));
        item.sliderFill->SetScale(Vector3(val, 1.0f, 1.0f));
    }

    void UpdateItemHighlight()
    {
        if (!m_visible) return;
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            float gamma = (i == m_selectedIndex) ? 1.0f : m_unselectedAlpha;

            if (m_items[i].polygon)
            {
                m_items[i].polygon->SetAlpha(gamma);
                if (!m_items[i].isSlider)
                {
                    float s = (i == m_selectedIndex) ? 1.1f : 1.0f;
                    m_items[i].polygon->SetScale(Vector3(s, s, s));
                }
            }
            if (m_items[i].sliderFill)
            {
                m_items[i].sliderFill->SetAlpha(gamma);
            }
        }
    }

    float m_bgX = 0, m_bgY = 0, m_bgW = 0, m_bgH = 0;
    std::wstring m_bgTexture;
    Polygon2D* m_bgPolygon = nullptr;
    float m_bgAlpha = 0.5f;

    float m_titleX = 0, m_titleY = 0, m_titleW = 0, m_titleH = 0;
    std::wstring m_titleTexture;
    Polygon2D* m_titlePolygon = nullptr;

    std::vector<MenuItem> m_items;
    size_t m_selectedIndex = 0;
    float m_unselectedAlpha = 0.5f;

    bool m_created = false;
    bool m_visible = false;
};

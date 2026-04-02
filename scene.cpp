#include "main.h"
#include "manager.h"
#include "renderer.h"
#include "scene.h"
#include "game.h"
#include "camera.h"


void Scene::Init()
{

}


void Scene::Uninit()
{
	for (int i = 0; i < LAYER_MAX; i++)
	{
		for (auto& gameObject : m_gameObject[i])
		{
			gameObject->Uninit();
		}
		m_gameObject[i].clear();
	}
}


void Scene::Update()
{
	// 全オブジェクトの更新を先に行う
        // 更新ループ
    for (int i = 0; i < LAYER_MAX; i++) {
        for (auto& gameObject : m_gameObject[i]) {
            gameObject->Update();
        }
    }
    // 保留中のオブジェクトを追加
    for (auto& pair : m_pendingObjects) {
        int layer = pair.first;
        auto& obj = pair.second;
        m_gameObject[layer].push_back(std::move(obj));
    }
    m_pendingObjects.clear();

	// 全更新が終わった後に削除処理を行う
	for (int i = 0; i < LAYER_MAX; i++)
	{
		auto& v = m_gameObject[i];
		v.erase(std::remove_if(v.begin(), v.end(),
			[](const std::unique_ptr<GameObject>& o) { return o->Destroy(); }), v.end());
	}
}

void Scene::Draw()
{
    Camera* camera = GetGameObject<Camera>();

    for (int i = 0; i < LAYER_MAX; i++)
    {
        if (i == LAYER_UI) {
            std::sort(
                m_gameObject[i].begin(), m_gameObject[i].end(),
                [](const std::unique_ptr<GameObject>& obj1, const std::unique_ptr<GameObject>& obj2) {
                    // 非アクティブは最後尾へ
                    if (obj1->IsActive() != obj2->IsActive())
                        return obj1->IsActive();
                    return obj1->GetPriority() < obj2->GetPriority();
                }
            );
        }
        else if (camera != nullptr) {
            std::sort(
                m_gameObject[i].begin(), m_gameObject[i].end(),
                [camera](const std::unique_ptr<GameObject>& obj1, const std::unique_ptr<GameObject>& obj2) {
                    // 非アクティブは最後尾へ（距離計算をスキップ）
                    if (obj1->IsActive() != obj2->IsActive())
                        return obj1->IsActive();
                    return obj1->GetDistance(camera->GetPosition()) >
                        obj2->GetDistance(camera->GetPosition());
                }
            );
        }
    }

    for (int i = 0; i < LAYER_MAX; i++)
    {
        for (auto& gameObject : m_gameObject[i])
        {
            // 非アクティブはソートで末尾に集まっているので、
                // 最初の非アクティブに当たったら残りもスキップ
            if (!gameObject->IsActive()) break;
            gameObject->Draw();
        }
    }
}

#pragma once

#include <map>
#include <string>
#include <memory>
#include <type_traits> // For std::is_base_of

class Scene; // Forward declaration
class Engine; // Forward declaration

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void Initialize(Engine* engine);

    template<typename T>
    void AddScene(const std::string& name) {
        static_assert(std::is_base_of<Scene, T>::value, "T must be a descendant of Scene");
        auto newScene = std::make_unique<T>();
        m_scenes[name] = std::move(newScene);
    }

    void LoadScene(const std::string& name);

    void Update(float deltaTime);
    void Draw();
    void Shutdown();
    void OnResize(int width, int height);

    Scene* GetActiveScene() { return m_activeScene; }

private:
    Engine* m_engine; // Pointer back to the engine
    std::map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene;
};

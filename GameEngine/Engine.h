#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "GameWorld.h"
#include "EventSystem.h"
#include "EntityFactory.h"
#include <chrono>
#include <iostream>
#include <mutex>

class Engine : public IEventListener {
private:
    GameWorld world;
    bool running = false;
    std::mutex worldMutex;

    Engine() {}
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

public:
    static Engine& getInstance() {
        static Engine instance;
        return instance;
    }

    GameWorld& getWorld() { return world; }
    std::mutex& getMutex() { return worldMutex; }

    void init() {
        running = true;

        
        EventManager::getInstance().subscribe("PlayerJoined", this);
        EventManager::getInstance().subscribe("PlayerLeft", this);
        EventManager::getInstance().subscribe("Collision", this);
        EventManager::getInstance().subscribe("ScoreUpdate", this);

        std::cout << "[ENGINE] Initialized\n";
    }

    void update(float dt) {
        std::lock_guard<std::mutex> lock(worldMutex);
        world.update(dt);
    }

    void render() {
        std::lock_guard<std::mutex> lock(worldMutex);
        world.render();
    }

    void shutdown() {
        running = false;
        std::cout << "[ENGINE] Shutting down\n";
    }

    bool isRunning() const { return running; }

   
    void onEvent(const Event& e) override {
        if (e.type == "PlayerJoined") {
            const auto& ev = static_cast<const PlayerJoinedEvent&>(e);
            std::cout << "[EVENT] Player joined: " << ev.username << " (id=" << ev.playerId << ")\n";
        }
        else if (e.type == "PlayerLeft") {
            const auto& ev = static_cast<const PlayerLeftEvent&>(e);
            std::cout << "[EVENT] Player left: id=" << ev.playerId << "\n";
        }
        else if (e.type == "Collision") {
            const auto& ev = static_cast<const CollisionEvent&>(e);
            std::cout << "[EVENT] Collision: entity " << ev.entityA << " hit " << ev.entityB << "\n";
        }
        else if (e.type == "ScoreUpdate") {
            const auto& ev = static_cast<const ScoreUpdateEvent&>(e);
            std::cout << "[EVENT] Score update: player " << ev.playerId << " -> " << ev.newScore << "\n";
        }
    }
};
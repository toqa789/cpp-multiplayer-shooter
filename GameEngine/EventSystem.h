#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <iostream>


struct Event {
    std::string type;
    Event(const std::string& t) : type(t) {}
    virtual ~Event() {}
};

struct PlayerJoinedEvent : public Event {
    int playerId;
    std::string username;
    PlayerJoinedEvent(int id, const std::string& name)
        : Event("PlayerJoined"), playerId(id), username(name) {
    }
};

struct PlayerLeftEvent : public Event {
    int playerId;
    PlayerLeftEvent(int id) : Event("PlayerLeft"), playerId(id) {}
};

struct CollisionEvent : public Event {
    int entityA, entityB;
    CollisionEvent(int a, int b) : Event("Collision"), entityA(a), entityB(b) {}
};

struct ScoreUpdateEvent : public Event {
    int playerId;
    int newScore;
    ScoreUpdateEvent(int id, int score) : Event("ScoreUpdate"), playerId(id), newScore(score) {}
};

struct IEventListener {
    virtual void onEvent(const Event& e) = 0;
    virtual ~IEventListener() {}
};


class EventManager {
private:
    std::map<std::string, std::vector<IEventListener*>> listeners;

    EventManager() {}
    EventManager(const EventManager&) = delete;

public:
    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    void subscribe(const std::string& eventType, IEventListener* listener) {
        listeners[eventType].push_back(listener);
    }

    void unsubscribe(const std::string& eventType, IEventListener* listener) {
        auto& vec = listeners[eventType];
        vec.erase(std::remove(vec.begin(), vec.end(), listener), vec.end());
    }

    void emit(const Event& e) {
        auto it = listeners.find(e.type);
        if (it != listeners.end()) {
            for (auto* listener : it->second)
                listener->onEvent(e);
        }
    }
};
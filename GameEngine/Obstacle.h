#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Entity.h"
#include <sstream>

class Obstacle : public Entity {
public:
    Obstacle(int id, float x, float y)
        : Entity(id, x, y, 999) {
    } 

    void update(float dt) override {
        
    }

    void render() const override {
        std::cout << "Obstacle[" << id << "] at (" << (int)x << "," << (int)y << ")\n";
    }

    void onCollision(Entity* other) override {
        
    }

    std::string serialize() const override {
        return "OBSTACLE," +
            std::to_string(id) + "," +
            std::to_string((int)x) + "," +
            std::to_string((int)y);
    }

    void deserialize(const std::string& data) override {
        std::istringstream ss(data);
        std::string t;
        char comma;
        std::getline(ss, t, ',');
        ss >> id >> comma >> x >> comma >> y;
    }
};

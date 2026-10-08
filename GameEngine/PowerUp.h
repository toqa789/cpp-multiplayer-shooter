#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Entity.h"
#include <sstream>

class PowerUp : public Entity {
private:
    int healAmount;
    std::string type;

public:
    PowerUp(int id, float x, float y)
        : Entity(id, x, y, 1), healAmount(25), type("HEALTH") {
    }

    void update(float dt) override {
        
    }

    void render() const override {
        std::cout << "PowerUp[" << id << "] at (" << (int)x << "," << (int)y << ")\n";
    }

    void onCollision(Entity* other) override {
        active = false; 
    }

    std::string serialize() const override {
        return "POWERUP," +
            std::to_string(id) + "," +
            std::to_string((int)x) + "," +
            std::to_string((int)y) + "," +
            std::to_string(active ? 1 : 0);
    }

    void deserialize(const std::string& data) override {
        std::istringstream ss(data);
        std::string t;
        char comma;
        int a;
        std::getline(ss, t, ',');
        ss >> id >> comma >> x >> comma >> y >> comma >> a;
        active = (a == 1);
    }

    int getHealAmount() const { return healAmount; }
    const std::string& getType() const { return type; }
};

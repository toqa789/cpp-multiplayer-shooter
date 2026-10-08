#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Entity.h"
#include <sstream>

class Projectile : public Entity {
private:
    int ownerId;  
    int damage;

public:
    Projectile(int id, float x, float y, int ownerId, float vx = 25.0f, float vy = 0.0f)
        : Entity(id, x, y, 1), ownerId(ownerId), damage(25) {
        this->vx = vx;
        this->vy = vy;
    }

    void update(float dt) override {
        x += vx * dt;
        y += vy * dt;

      
        if (x < 0 || x > 79 || y < 0 || y > 19)
            active = false;
    }

    void render() const override {
        std::cout << "  Bullet[" << id << "] at (" << (int)x << "," << (int)y << ") owner:" << ownerId << "\n";
    }

    void onCollision(Entity* other) override {
        active = false;
    }

    std::string serialize() const override {
        return "BULLET," +
            std::to_string(id) + "," +
            std::to_string((int)x) + "," +
            std::to_string((int)y) + "," +
            std::to_string(ownerId);
    }

    void deserialize(const std::string& data) override {
        
        std::istringstream ss(data);
        std::string type;
        char comma;
        std::getline(ss, type, ',');
        ss >> id >> comma >> x >> comma >> y >> comma >> ownerId;
    }

    int getOwnerId() const { return ownerId; }
    int getDamage() const { return damage; }
};
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <iostream>
#include <string>
#include <sstream>

class Entity {
protected:
    int id;
    float x, y;
    float vx, vy;
    int health;
    bool active;

public:
    Entity(int id, float x, float y, int health = 100)
        : id(id), x(x), y(y), vx(0), vy(0), health(health), active(true) {
    }

    virtual void update(float dt) = 0;

    virtual void render() const {
        std::cout << "Entity " << id << " at (" << x << "," << y << ") HP:" << health << "\n";
    }

    virtual void onCollision(Entity* other) {}

    virtual std::string serialize() const {
        return std::to_string(id) + "," +
            std::to_string((int)x) + "," +
            std::to_string((int)y) + "," +
            std::to_string((int)vx) + "," +
            std::to_string((int)vy) + "," +
            std::to_string(health) + "," +
            std::to_string(active ? 1 : 0);
    }

    virtual void deserialize(const std::string& data) {
        std::istringstream ss(data);
        char comma;
        int a;
        ss >> id >> comma >> x >> comma >> y >> comma
            >> vx >> comma >> vy >> comma >> health >> comma >> a;
        active = (a == 1);
    }

    bool isActive() const { return active; }
    void setActive(bool a) { active = a; }

    int getId() const { return id; }
    float getX() const { return x; }
    float getY() const { return y; }
    int getHealth() const { return health; }

    void setVelocity(float vx_, float vy_) { vx = vx_; vy = vy_; }
    void setPosition(float x_, float y_) { x = x_; y = y_; }
    void setHealth(int h) { health = h; if (health > 0) active = true; }
    void takeDamage(int dmg) { health -= dmg; if (health <= 0) { health = 0; active = false; } }

    virtual ~Entity() {}
};
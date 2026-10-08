#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Entity.h"
#include <string>
#include <vector>

class Player : public Entity {
private:
    std::string username;
    int score;
    std::vector<std::string> inventory;
    int ping;
    int connectionId;
    bool alive;
    std::string state;

    
    float facingX = 1.0f;  
    float facingY = 0.0f;

public:
    Player(int id, float x, float y, const std::string& username = "Player", int connId = -1)
        : Entity(id, x, y, 100),
        username(username), score(0), ping(0),
        connectionId(connId), alive(true), state("idle") {
    }

    void update(float dt) override {
        if (!active) return;
        x += vx * dt;
        y += vy * dt;

        if (x < 1)  x = 1;
        if (x > 78) x = 78;
        if (y < 1)  y = 1;
        if (y > 18) y = 18;

        vx *= 0.85f;
        vy *= 0.85f;
    }

    void render() const override {
        std::cout << "[P" << id << "] " << username
            << " at (" << (int)x << "," << (int)y << ")"
            << " HP:" << health
            << " Score:" << score
            << " State:" << state << "\n";
    }

    void onCollision(Entity* other) override {}

    std::string serialize() const override {
        
        return "PLAYER," +
            std::to_string(id) + "," +
            std::to_string((int)x) + "," +
            std::to_string((int)y) + "," +
            std::to_string(health) + "," +
            std::to_string(score) + "," +
            username + "," +
            std::to_string(connectionId) + "," +
            std::to_string(active ? 1 : 0);
    }

    void deserialize(const std::string& data) override {
       
        std::istringstream ss(data);
        std::string type;
        char comma;
        int a;
        std::getline(ss, type, ',');
        ss >> id >> comma >> x >> comma >> y >> comma
            >> health >> comma >> score >> comma;
        std::getline(ss, username, ',');
        ss >> connectionId >> comma >> a;
        active = (a == 1);
    }

    void move(float dx, float dy) {
        vx = dx;
        vy = dy;
        
        if (dx > 0) { facingX = 1.0f; facingY = 0.0f; }
        else if (dx < 0) { facingX = -1.0f; facingY = 0.0f; }
        else if (dy > 0) { facingX = 0.0f; facingY = 1.0f; }
        else if (dy < 0) { facingX = 0.0f; facingY = -1.0f; }
        state = "moving";
    }

    void attack() { state = "shooting"; }

    void pickItem(const std::string& item) {
        inventory.push_back(item);
    }

    void sendInputToServer() {}

    void addScore(int pts) { score += pts; }
    void setState(const std::string& s) { state = s; }

    
    const std::string& getUsername()  const { return username; }
    int  getScore()                   const { return score; }
    int  getConnectionId()            const { return connectionId; }
    const std::string& getState()     const { return state; }
    float getFacingX()                const { return facingX; }
    float getFacingY()                const { return facingY; }
    void setConnectionId(int id) { connectionId = id; }
};
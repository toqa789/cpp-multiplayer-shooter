#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <memory>
#include <string>
#include "Player.h"
#include "Projectile.h"
#include "PowerUp.h"
#include "Obstacle.h"


class EntityFactory {
private:
    static int nextId;

public:
    static std::shared_ptr<Player> createPlayer(
        const std::string& username, float x, float y, int connId = -1)
    {
        return std::make_shared<Player>(nextId++, x, y, username, connId);
    }

    static std::shared_ptr<Projectile> createProjectile(
        float x, float y, int ownerId, float vx = 25.0f, float vy = 0.0f)
    {
        return std::make_shared<Projectile>(nextId++, x, y, ownerId, vx, vy);
    }

    static std::shared_ptr<PowerUp> createPowerUp(float x, float y)
    {
        return std::make_shared<PowerUp>(nextId++, x, y);
    }

    static std::shared_ptr<Obstacle> createObstacle(float x, float y)
    {
        return std::make_shared<Obstacle>(nextId++, x, y);
    }
};

inline int EntityFactory::nextId = 1;
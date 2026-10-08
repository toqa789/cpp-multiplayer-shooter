#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include "Entity.h"
#include "Player.h"
#include "Projectile.h"
#include "PowerUp.h"
#include "Obstacle.h"
#include "EventSystem.h"


class PhysicsEngine {
public:
    void updatePositions(std::vector<std::shared_ptr<Entity>>& entities, float dt) {
        for (auto& e : entities)
            if (e->isActive())
                e->update(dt);
    }

    
    bool checkCollision(Entity* a, Entity* b) {
        float ax = a->getX(), ay = a->getY();
        float bx = b->getX(), by = b->getY();
        float dx = std::abs(ax - bx);
        float dy = std::abs(ay - by);

       
        bool aIsObs = dynamic_cast<Obstacle*>(a) != nullptr;
        bool bIsObs = dynamic_cast<Obstacle*>(b) != nullptr;
        float threshold = (aIsObs || bIsObs) ? 2.5f : 1.5f;

        return dx < threshold && dy < threshold;
    }

    void detectCollisions(std::vector<std::shared_ptr<Entity>>& entities,
        std::vector<std::shared_ptr<Player>>& players) {
        for (size_t i = 0; i < entities.size(); i++) {
            for (size_t j = i + 1; j < entities.size(); j++) {
                Entity* a = entities[i].get();
                Entity* b = entities[j].get();
                if (!a->isActive() || !b->isActive()) continue;
                if (checkCollision(a, b))
                    resolveCollision(a, b, players);
            }
        }
    }

    void resolveCollision(Entity* a, Entity* b,
        std::vector<std::shared_ptr<Player>>& players) {
        
        Projectile* proj = nullptr;
        Player* victim = nullptr;

        proj = dynamic_cast<Projectile*>(a);
        victim = dynamic_cast<Player*>(b);
        if (!proj || !victim) {
            proj = dynamic_cast<Projectile*>(b);
            victim = dynamic_cast<Player*>(a);
        }

        if (proj && victim) {
            if (proj->getOwnerId() == victim->getId()) return;
            victim->takeDamage(proj->getDamage());
            proj->onCollision(victim);

            for (auto& p : players) {
                if (p->getId() == proj->getOwnerId()) {
                    p->addScore(10);
                    EventManager::getInstance().emit(
                        ScoreUpdateEvent(p->getId(), p->getScore()));
                    break;
                }
            }
            EventManager::getInstance().emit(
                CollisionEvent(victim->getId(), proj->getId()));
            std::cout << "[COLLISION] Bullet hit Player " << victim->getId()
                << " | HP remaining: " << victim->getHealth() << "\n";
            return;
        }

       
        Projectile* projObs = nullptr;
        Obstacle* obs = nullptr;
        projObs = dynamic_cast<Projectile*>(a);
        obs = dynamic_cast<Obstacle*>(b);
        if (!projObs || !obs) {
            projObs = dynamic_cast<Projectile*>(b);
            obs = dynamic_cast<Obstacle*>(a);
        }
        if (projObs && obs) {
            projObs->setActive(false);
            std::cout << "[OBSTACLE] Bullet " << projObs->getId() << " blocked by obstacle\n";
            return;
        }

        
        Player* playerObs = nullptr;
        Obstacle* obsP = nullptr;
        playerObs = dynamic_cast<Player*>(a);
        obsP = dynamic_cast<Obstacle*>(b);
        if (!playerObs || !obsP) {
            playerObs = dynamic_cast<Player*>(b);
            obsP = dynamic_cast<Obstacle*>(a);
        }
        if (playerObs && obsP) {
            float dx = playerObs->getX() - obsP->getX();
            float dy = playerObs->getY() - obsP->getY();
            if (std::abs(dx) > std::abs(dy)) {
                playerObs->setPosition(obsP->getX() + (dx > 0 ? 3.0f : -3.0f), playerObs->getY());
            }
            else {
                playerObs->setPosition(playerObs->getX(), obsP->getY() + (dy > 0 ? 3.0f : -3.0f));
            }
            playerObs->setVelocity(0, 0);
            return;
        }

       
        Player* collector = nullptr;
        PowerUp* powerup = nullptr;
        collector = dynamic_cast<Player*>(a);
        powerup = dynamic_cast<PowerUp*>(b);
        if (!collector || !powerup) {
            collector = dynamic_cast<Player*>(b);
            powerup = dynamic_cast<PowerUp*>(a);
        }
        if (collector && powerup && powerup->isActive()) {
            int healed = powerup->getHealAmount();
            int newHp = collector->getHealth() + healed;
            if (newHp > 100) newHp = 100;
            collector->setHealth(newHp);
            powerup->onCollision(collector); 
            collector->addScore(5);          
            std::cout << "[POWERUP] Player " << collector->getId()
                << " collected HP+" << healed
                << " | HP now: " << collector->getHealth() << "\n";
        }
    }
};
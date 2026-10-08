#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vector>
#include <memory>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include "Entity.h"
#include "Player.h"
#include "Projectile.h"
#include "PowerUp.h"
#include "Obstacle.h"
#include "PhysicsEngine.h"
#include "EntityFactory.h"

class GameWorld {
public:
    std::vector<std::shared_ptr<Entity>>  entities;
    std::vector<std::shared_ptr<Player>>  players;

private:
    PhysicsEngine physics;
    int worldWidth = 80;
    int worldHeight = 20;

public:
    void add(std::shared_ptr<Entity> e) {
        entities.push_back(e);
        auto p = std::dynamic_pointer_cast<Player>(e);
        if (p) players.push_back(p);
    }

    void removePlayer(int connId) {
        players.erase(
            std::remove_if(players.begin(), players.end(),
                [connId](const std::shared_ptr<Player>& p) {
                    return p->getConnectionId() == connId;
                }),
            players.end()
        );
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [connId](const std::shared_ptr<Entity>& e) {
                    Player* p = dynamic_cast<Player*>(e.get());
                    return p && p->getConnectionId() == connId;
                }),
            entities.end()
        );
    }

    Player* getPlayerByConnId(int connId) {
        for (auto& p : players)
            if (p->getConnectionId() == connId) return p.get();
        return nullptr;
    }

    void spawnProjectile(int ownerId, float x, float y, float vx = 25.0f, float vy = 0.0f) {
        entities.push_back(EntityFactory::createProjectile(x, y, ownerId, vx, vy));
    }

    void spawnInitialItems() {
        
        entities.push_back(EntityFactory::createObstacle(25.0f, 7.0f));
        entities.push_back(EntityFactory::createObstacle(26.0f, 7.0f));
        entities.push_back(EntityFactory::createObstacle(27.0f, 7.0f));

        entities.push_back(EntityFactory::createObstacle(45.0f, 12.0f));
        entities.push_back(EntityFactory::createObstacle(46.0f, 12.0f));
        entities.push_back(EntityFactory::createObstacle(47.0f, 12.0f));

        entities.push_back(EntityFactory::createObstacle(38.0f, 5.0f));
        entities.push_back(EntityFactory::createObstacle(38.0f, 6.0f));
        entities.push_back(EntityFactory::createObstacle(38.0f, 7.0f));

       
        entities.push_back(EntityFactory::createPowerUp(15.0f, 5.0f));
        entities.push_back(EntityFactory::createPowerUp(62.0f, 14.0f));
        entities.push_back(EntityFactory::createPowerUp(38.0f, 15.0f));
    }

    void update(float dt) {
       
        physics.updatePositions(entities, dt);

       
        physics.detectCollisions(entities, players);

       
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [](const std::shared_ptr<Entity>& e) {
                   
                    if (dynamic_cast<Player*>(e.get())) return false;
                    return !e->isActive();
                }),
            entities.end()
        );
    }

    // ─── Console Grid Renderer ─────────────────────────────────
    void render() const {
       
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        // Build grid
        std::vector<std::string> grid(worldHeight, std::string(worldWidth, '.'));

        // Draw borders
        for (int x = 0; x < worldWidth; x++) {
            grid[0][x] = '-';
            grid[worldHeight - 1][x] = '-';
        }
        for (int y = 0; y < worldHeight; y++) {
            grid[y][0] = '|';
            grid[y][worldWidth - 1] = '|';
        }

        // Place entities on grid
        for (auto& e : entities) {
            if (!e->isActive()) continue;
            int gx = (std::max)(1, (std::min)(worldWidth - 2, (int)e->getX()));
            int gy = (std::max)(1, (std::min)(worldHeight - 2, (int)e->getY()));

            Player* p = dynamic_cast<Player*>(e.get());
            Projectile* b = dynamic_cast<Projectile*>(e.get());
            PowerUp* pu = dynamic_cast<PowerUp*>(e.get());
            Obstacle* obs = dynamic_cast<Obstacle*>(e.get());

            if (p) {
                if (gx > 1) grid[gy][gx - 1] = 'P';
                grid[gy][gx] = '0' + (p->getId() % 10);
            }
            else if (b)   grid[gy][gx] = '*';
            else if (pu)  grid[gy][gx] = '+';
            else if (obs) grid[gy][gx] = '#';
        }

        // Print grid
        std::cout << "\n";
        for (auto& row : grid)
            std::cout << row << "\n";

        // Print player stats below map
        std::cout << "\n";
        for (auto& p : players) {
            std::cout << "  [P" << p->getId() << "] "
                << p->getUsername()
                << "  HP:" << p->getHealth()
                << "  Score:" << p->getScore()
                << (p->isActive() ? "" : "  [DEAD]") << "\n";
        }
        std::cout << "\nControls: A=left  D=right  W=up  S=down  SPACE=shoot\n";
    }

    // Check if game is over (only 1 or 0 players alive)
    bool isGameOver() const {
        int alive = 0;
        for (auto& p : players)
            if (p->isActive()) alive++;
        return players.size() > 1 && alive <= 1;
    }

    Player* getWinner() const {
        for (auto& p : players)
            if (p->isActive()) return p.get();
        return nullptr;
    }

    
    std::string serializeState() const {
        std::ostringstream oss;
        oss << "WORLD_STATE|";

        
        for (auto& p : players)
            oss << p->serialize() << ";";

        
        for (auto& e : entities) {
            if (dynamic_cast<Player*>(e.get())) continue;
            if (e->isActive())
                oss << e->serialize() << ";";
        }

        
        if (isGameOver()) {
            Player* winner = getWinner();
            oss << "GAMEOVER,";
            if (winner)
                oss << winner->getUsername() << "," << winner->getScore();
            else
                oss << "draw,0";
            oss << ";";
        }

        return oss.str();
    }

   
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);
};
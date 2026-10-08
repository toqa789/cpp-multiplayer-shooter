#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "GameWorld.h"
#include <fstream>
#include <sstream>
#include <iostream>


inline void GameWorld::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SAVE] Failed to open " << filename << "\n";
        return;
    }

    file << "# Game State Save\n";
    for (auto& e : entities) {
        file << e->serialize() << "\n";
    }

    std::cout << "[SAVE] World saved to " << filename << "\n";
}


inline void GameWorld::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[LOAD] Failed to open " << filename << "\n";
        return;
    }

    entities.clear();
    players.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string type;
        std::getline(ss, type, ',');

        if (type == "PLAYER") {
           
            int id, x, y, health, score, connId;
            std::string username;
            char comma;
            ss >> id >> comma >> x >> comma >> y >> comma
                >> health >> comma >> score >> comma;
            std::getline(ss, username, ',');
            ss >> connId;

            auto p = std::make_shared<Player>(id, (float)x, (float)y, username, connId);
            add(p);
        }
    }

    std::cout << "[LOAD] World loaded from " << filename << "\n";
}


inline void saveLeaderboard(const std::vector<std::shared_ptr<Player>>& players,
    const std::string& filename = "leaderboard.txt") {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "=== LEADERBOARD ===\n";

    
    auto sorted = players;
    std::sort(sorted.begin(), sorted.end(),
        [](const std::shared_ptr<Player>& a, const std::shared_ptr<Player>& b) {
            return a->getScore() > b->getScore();
        });

    int rank = 1;
    for (auto& p : sorted) {
        file << rank++ << ". " << p->getUsername()
            << "  Score: " << p->getScore()
            << "  HP: " << p->getHealth() << "\n";
    }

    std::cout << "[LEADERBOARD] Saved to " << filename << "\n";
}
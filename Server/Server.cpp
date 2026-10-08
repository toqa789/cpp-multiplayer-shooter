#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")


#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cmath>


#include "Engine.h"
#include "EntityFactory.h"
#include "CommandsImpl.h"
#include "Persistence.h"
#include "EventSystem.h"

using namespace std::chrono;


struct ClientInfo {
    SOCKET socket;
    int connId;
    std::string username;
    bool connected;
};

std::vector<ClientInfo> clients;
std::mutex clientsMutex;
int nextConnId = 1;


void broadcastState() {
    Engine& engine = Engine::getInstance();
    std::string state = engine.getWorld().serializeState() + "\n";

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& c : clients) {
        if (c.connected) {
            int sent = send(c.socket, state.c_str(), (int)state.size(), 0);
            if (sent == SOCKET_ERROR)
                c.connected = false;
        }
    }
}


void handleClient(ClientInfo info) {
    Engine& engine = Engine::getInstance();
    char buffer[1025];

    std::cout << "[SERVER] Client " << info.connId << " (" << info.username << ") handler started\n";

    try {
        while (true) {
#pragma warning(suppress: 6386)
            int bytes = recv(info.socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                std::cout << "[SERVER] Client " << info.connId << " disconnected\n";

                {
                    std::lock_guard<std::mutex> lock(engine.getMutex());
                    engine.getWorld().removePlayer(info.connId);
                }

                EventManager::getInstance().emit(PlayerLeftEvent(info.connId));

                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    for (auto& c : clients)
                        if (c.connId == info.connId)
                            c.connected = false;
                }

                broadcastState();
                closesocket(info.socket);
                return;
            }

            buffer[bytes] = '\0';
            std::string cmd(buffer);

            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r' || cmd.back() == ' '))
                cmd.pop_back();

            std::cout << "[SERVER] From client " << info.connId << ": " << cmd << "\n";

            std::lock_guard<std::mutex> lock(engine.getMutex());
            Player* player = engine.getWorld().getPlayerByConnId(info.connId);

            if (!player) {
                std::cout << "[SERVER] Warning: no player found for connId " << info.connId << "\n";
                continue;
            }

            if (cmd == "SHOOT") {
                float fx = player->getFacingX(); 
                float fy = player->getFacingY();  
               
                if (fx == 0.0f && fy == 0.0f) fx = 1.0f;
                engine.getWorld().spawnProjectile(
                    player->getId(),
                    player->getX() + fx,
                    player->getY() + fy,
                    fx * 25.0f, fy * 25.0f
                );
                player->attack();
            }
            else {
                auto command = parseCommand(cmd);
                if (command) {
                    command->execute(player);
                }
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "[SERVER] Exception in client handler " << info.connId
            << ": " << ex.what() << "\n";
        closesocket(info.socket);
    }
    catch (...) {
        std::cerr << "[SERVER] Unknown exception in client handler " << info.connId << "\n";
        closesocket(info.socket);
    }
}


void gameLoopThread() {
    Engine& engine = Engine::getInstance();
    auto lastTime = high_resolution_clock::now();
    int frame = 0;

    try {
        while (engine.isRunning()) {
            auto now = high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            engine.update(dt);
            broadcastState();

            if (frame % 300 == 0 && frame > 0) {
                std::lock_guard<std::mutex> lock(engine.getMutex());
                engine.getWorld().saveToFile("gamestate.txt");
                saveLeaderboard(engine.getWorld().players);
            }

            frame++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "[SERVER] Game loop exception: " << ex.what() << "\n";
    }
}

// ─── Main ─────────────────────────────────────────────────────
int main() {
   
    WSADATA wsa;
#pragma warning(suppress: 6387)
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[SERVER] WSAStartup failed\n";
        return 1;
    }
    (void)wsa; 

    
    Engine& engine = Engine::getInstance();
    engine.init();
    engine.getWorld().spawnInitialItems();

    
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "[SERVER] socket() failed\n";
        return 1;
    }

   
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[SERVER] bind() failed\n";
        return 1;
    }

    listen(serverSock, 10);
    std::cout << "[SERVER] Running on port 54000. Waiting for players...\n";

    std::thread gameLoop(gameLoopThread);
    gameLoop.detach();

    while (true) {
        SOCKET clientSock = accept(serverSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) continue;

        char nameBuf[65] = {};
        int nb = recv(clientSock, nameBuf, sizeof(nameBuf) - 1, 0);
        std::string username = (nb > 0) ? std::string(nameBuf, nb) : "Player";

        while (!username.empty() && (username.back() == '\n' || username.back() == '\r'))
            username.pop_back();

        int connId = nextConnId++;

        
        const float spawnPoints[][2] = {
            { 5.0f,  5.0f },  
            { 72.0f, 14.0f }, 
            { 72.0f,  5.0f }, 
            { 5.0f,  14.0f }, 
            { 38.0f,  2.0f },
            { 38.0f, 17.0f }, 
            { 20.0f,  9.0f }, 
            { 58.0f,  9.0f }, 
        };
        int spawnIndex = (connId - 1) % 8;
        float spawnX = spawnPoints[spawnIndex][0];
        float spawnY = spawnPoints[spawnIndex][1];

        auto player = EntityFactory::createPlayer(username, spawnX, spawnY, connId);
        int playerId = player->getId();

        {
            std::lock_guard<std::mutex> lock(engine.getMutex());
            engine.getWorld().add(player);
        }

        ClientInfo info{ clientSock, connId, username, true };
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.emplace_back(info);
        }

        std::cout << "[SERVER] Player '" << username << "' connected (connId=" << connId << ")\n";

        EventManager::getInstance().emit(PlayerJoinedEvent(playerId, username));

        std::thread t(handleClient, std::move(info));
        t.detach();
    }

    engine.shutdown();
    saveLeaderboard(engine.getWorld().players, "final_leaderboard.txt");
    WSACleanup();
    return 0;
}
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <SFML/Graphics.hpp>

#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ============================================================
// NETWORKING
// ============================================================

SOCKET sock = INVALID_SOCKET;

atomic<bool> connected(true);
atomic<bool> gameOver(false);

mutex worldMutex;


// ============================================================
// DATA STRUCTURES
// ============================================================

struct PlayerInfo
{
    int id;
    int x;
    int y;
    int health;
    int score;
    string username;
    int connectionId;
    bool active;
};

struct BulletInfo
{
    int id;
    int x;
    int y;
    int ownerId;
};

struct PowerUpInfo
{
    int id;
    int x;
    int y;
    bool active;
};

struct ObstacleInfo
{
    int id;
    int x;
    int y;
};

struct WorldState
{
    vector<PlayerInfo> players;
    vector<BulletInfo> bullets;
    vector<PowerUpInfo> powerUps;
    vector<ObstacleInfo> obstacles;

    string winner;
    int winnerScore = 0;

    bool gameOver = false;
};

WorldState currentWorld;


// ============================================================
// SEND COMMAND
// ============================================================

void sendCommand(const string& command)
{
    if (!connected)
        return;

    int result = send(
        sock,
        command.c_str(),
        static_cast<int>(command.size()),
        0
    );

    if (result == SOCKET_ERROR)
    {
        connected = false;
    }
}


// ============================================================
// PARSE WORLD STATE
// ============================================================

WorldState parseWorldState(const string& raw)
{
    WorldState newWorld;

    // Find the WORLD_STATE| part
    size_t separator = raw.find('|');

    string data;

    if (separator != string::npos)
        data = raw.substr(separator + 1);
    else
        data = raw;

    stringstream ss(data);
    string token;

    while (getline(ss, token, ';'))
    {
        if (token.empty())
            continue;

        stringstream ts(token);

        string type;
        getline(ts, type, ',');

        // ----------------------------------------------------
        // PLAYER
        // ----------------------------------------------------

        if (type == "PLAYER")
        {
            PlayerInfo player;

            char comma;

            ts >> player.id
                >> comma
                >> player.x
                >> comma
                >> player.y
                >> comma
                >> player.health
                >> comma
                >> player.score
                >> comma;

            getline(ts, player.username, ',');

            ts >> player.connectionId
                >> comma;

            int active;

            ts >> active;

            player.active = (active == 1);

            newWorld.players.push_back(player);
        }

        // ----------------------------------------------------
        // BULLET
        // ----------------------------------------------------

        else if (type == "BULLET")
        {
            BulletInfo bullet;

            char comma;

            ts >> bullet.id
                >> comma
                >> bullet.x
                >> comma
                >> bullet.y
                >> comma
                >> bullet.ownerId;

            newWorld.bullets.push_back(bullet);
        }

        // ----------------------------------------------------
        // POWERUP
        // ----------------------------------------------------

        else if (type == "POWERUP")
        {
            PowerUpInfo powerUp;

            char comma;

            int active;

            ts >> powerUp.id
                >> comma
                >> powerUp.x
                >> comma
                >> powerUp.y
                >> comma
                >> active;

            powerUp.active = (active == 1);

            newWorld.powerUps.push_back(powerUp);
        }

        // ----------------------------------------------------
        // OBSTACLE
        // ----------------------------------------------------

        else if (type == "OBSTACLE")
        {
            ObstacleInfo obstacle;

            char comma;

            ts >> obstacle.id
                >> comma
                >> obstacle.x
                >> comma
                >> obstacle.y;

            newWorld.obstacles.push_back(obstacle);
        }

        // ----------------------------------------------------
        // GAME OVER
        // ----------------------------------------------------

        else if (type == "GAMEOVER")
        {
            getline(ts, newWorld.winner, ',');

            ts >> newWorld.winnerScore;

            newWorld.gameOver = true;
        }
    }

    return newWorld;
}


// ============================================================
// RECEIVE THREAD
// ============================================================

void receiveLoop()
{
    char buffer[8192];

    string leftover;

    while (connected)
    {
        int bytes = recv(
            sock,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytes <= 0)
        {
            connected = false;
            return;
        }

        buffer[bytes] = '\0';

        leftover += string(buffer, bytes);

        size_t position;

        // TCP can split packets, so we wait for '\n'
        while ((position = leftover.find('\n')) != string::npos)
        {
            string packet = leftover.substr(0, position);

            leftover.erase(0, position + 1);

            if (packet.empty())
                continue;

            WorldState newWorld = parseWorldState(packet);

            {
                lock_guard<mutex> lock(worldMutex);

                currentWorld = newWorld;
            }

            if (newWorld.gameOver)
            {
                gameOver = true;
            }
        }
    }
}


// ============================================================
// WORLD COORDINATE SYSTEM
// ============================================================

// The server uses an 80 x 20 world.

constexpr float WORLD_WIDTH = 80.0f;
constexpr float WORLD_HEIGHT = 20.0f;

// Game area on the screen.

constexpr float GAME_WIDTH = 1120.0f;
constexpr float GAME_HEIGHT = 280.0f;

constexpr float GAME_LEFT = 40.0f;
constexpr float GAME_TOP = 200.0f;

// Since 80 / 20 = 4,
// GAME_WIDTH / GAME_HEIGHT is also 4.

constexpr float CELL_SIZE = GAME_WIDTH / WORLD_WIDTH;


// Convert server X coordinate to screen X.

float screenX(float worldX)
{
    return GAME_LEFT + worldX * CELL_SIZE;
}


// Convert server Y coordinate to screen Y.

float screenY(float worldY)
{
    return GAME_TOP + worldY * CELL_SIZE;
}


// ============================================================
// DRAW TEXT
// ============================================================

void drawText(
    sf::RenderWindow& window,
    sf::Font& font,
    const string& text,
    unsigned int size,
    float x,
    float y
)
{
    sf::Text drawableText(font, text, size);

    drawableText.setPosition({ x, y });

    window.draw(drawableText);
}


// ============================================================
// DRAW PLAYER
// ============================================================

void drawPlayer(
    sf::RenderWindow& window,
    sf::Font& font,
    const PlayerInfo& player,
    int playerNumber
)
{
    if (!player.active)
        return;

    float x = screenX(static_cast<float>(player.x));
    float y = screenY(static_cast<float>(player.y));

    // --------------------------------------------------------
    // Player color
    // --------------------------------------------------------

    vector<sf::Color> colors =
    {
        sf::Color(80, 170, 255),   // Blue
        sf::Color(255, 90, 90),    // Red
        sf::Color(255, 210, 70),   // Yellow
        sf::Color(90, 230, 160),   // Green
        sf::Color(200, 120, 255),  // Purple
        sf::Color(255, 150, 70),   // Orange
        sf::Color(80, 220, 220),   // Cyan
        sf::Color(255, 120, 190)   // Pink
    };

    sf::Color playerColor =
        colors[(playerNumber - 1) % colors.size()];

    // --------------------------------------------------------
    // Circle
    // --------------------------------------------------------

    sf::CircleShape circle(12.0f);

    circle.setOrigin({ 12.0f, 12.0f });

    circle.setPosition({ x, y });

    circle.setFillColor(playerColor);

    circle.setOutlineThickness(2.0f);

    circle.setOutlineColor(sf::Color::White);

    window.draw(circle);

    // --------------------------------------------------------
    // Player number
    // --------------------------------------------------------

    sf::Text numberText(
        font,
        "P" + to_string(playerNumber),
        12
    );

    numberText.setFillColor(sf::Color::Black);

    sf::FloatRect bounds = numberText.getLocalBounds();

    numberText.setOrigin({
        bounds.position.x + bounds.size.x / 2.0f,
        bounds.position.y + bounds.size.y / 2.0f
        });

    numberText.setPosition({ x, y });

    window.draw(numberText);

    // --------------------------------------------------------
    // Username
    // --------------------------------------------------------

    sf::Text nameText(
        font,
        player.username,
        16
    );

    nameText.setFillColor(sf::Color::White);

    sf::FloatRect nameBounds = nameText.getLocalBounds();

    nameText.setOrigin({
        nameBounds.position.x + nameBounds.size.x / 2.0f,
        nameBounds.position.y + nameBounds.size.y / 2.0f
        });

    nameText.setPosition({
        x,
        y - 30.0f
        });

    window.draw(nameText);
}


// ============================================================
// DRAW BULLET
// ============================================================

void drawBullet(
    sf::RenderWindow& window,
    const BulletInfo& bullet
)
{
    float x = screenX(static_cast<float>(bullet.x));
    float y = screenY(static_cast<float>(bullet.y));

    sf::RectangleShape shape({ 9.0f, 4.0f });

    shape.setOrigin({ 4.5f, 2.0f });

    shape.setPosition({ x, y });

    shape.setFillColor(sf::Color(255, 230, 100));

    window.draw(shape);
}


// ============================================================
// DRAW POWERUP
// ============================================================

void drawPowerUp(
    sf::RenderWindow& window,
    const PowerUpInfo& powerUp
)
{
    if (!powerUp.active)
        return;

    float x = screenX(static_cast<float>(powerUp.x));
    float y = screenY(static_cast<float>(powerUp.y));

    // Horizontal part

    sf::RectangleShape horizontal({ 20.0f, 6.0f });

    horizontal.setOrigin({ 10.0f, 3.0f });

    horizontal.setPosition({ x, y });

    horizontal.setFillColor(sf::Color(80, 255, 100));

    // Vertical part

    sf::RectangleShape vertical({ 6.0f, 20.0f });

    vertical.setOrigin({ 3.0f, 10.0f });

    vertical.setPosition({ x, y });

    vertical.setFillColor(sf::Color(80, 255, 100));

    window.draw(horizontal);

    window.draw(vertical);
}


// ============================================================
// DRAW OBSTACLE
// ============================================================

void drawObstacle(
    sf::RenderWindow& window,
    const ObstacleInfo& obstacle
)
{
    float x = screenX(static_cast<float>(obstacle.x));
    float y = screenY(static_cast<float>(obstacle.y));

    sf::RectangleShape block({ 14.0f, 14.0f });

    block.setOrigin({ 7.0f, 7.0f });

    block.setPosition({ x, y });

    block.setFillColor(sf::Color(110, 110, 120));

    block.setOutlineThickness(1.0f);

    block.setOutlineColor(sf::Color(170, 170, 180));

    window.draw(block);
}


// ============================================================
// DRAW GAME AREA
// ============================================================

void drawGameArea(
    sf::RenderWindow& window
)
{
    // Background

    sf::RectangleShape background(
        { GAME_WIDTH, GAME_HEIGHT }
    );

    background.setPosition({
        GAME_LEFT,
        GAME_TOP
        });

    background.setFillColor(
        sf::Color(25, 28, 35)
    );

    background.setOutlineThickness(3.0f);

    background.setOutlineColor(
        sf::Color(100, 105, 120)
    );

    window.draw(background);

    // Subtle grid

    for (int x = 1; x < 80; x++)
    {
        sf::RectangleShape line(
            { 1.0f, GAME_HEIGHT }
        );

        line.setPosition({
            GAME_LEFT + x * CELL_SIZE,
            GAME_TOP
            });

        line.setFillColor(
            sf::Color(35, 39, 48)
        );

        window.draw(line);
    }

    for (int y = 1; y < 20; y++)
    {
        sf::RectangleShape line(
            { GAME_WIDTH, 1.0f }
        );

        line.setPosition({
            GAME_LEFT,
            GAME_TOP + y * CELL_SIZE
            });

        line.setFillColor(
            sf::Color(35, 39, 48)
        );

        window.draw(line);
    }
}


// ============================================================
// DRAW HUD
// ============================================================

void drawHUD(
    sf::RenderWindow& window,
    sf::Font& font,
    const WorldState& world
)
{
    drawText(
        window,
        font,
        "MULTIPLAYER SHOOTER",
        30,
        40,
        30
    );

    drawText(
        window,
        font,
        "W A S D  -  Move       SPACE  -  Shoot       Q  -  Quit",
        17,
        40,
        80
    );

    float sidebarX = 40.0f;

    float y = 530.0f;

    drawText(
        window,
        font,
        "PLAYERS",
        22,
        sidebarX,
        y
    );

    y += 35.0f;

    int playerNumber = 1;

    for (const auto& player : world.players)
    {
        string status =
            "P" + to_string(playerNumber) +
            "  " +
            player.username +
            "    HP: " +
            to_string(player.health) +
            "    Score: " +
            to_string(player.score);

        if (!player.active)
            status += "  [DEAD]";

        drawText(
            window,
            font,
            status,
            17,
            sidebarX,
            y
        );

        y += 25.0f;

        playerNumber++;
    }
}


// ============================================================
// DRAW GAME OVER
// ============================================================

void drawGameOver(
    sf::RenderWindow& window,
    sf::Font& font,
    const WorldState& world
)
{
    sf::RectangleShape overlay(
        { 700.0f, 260.0f }
    );

    overlay.setPosition({
        290.0f,
        230.0f
        });

    overlay.setFillColor(
        sf::Color(15, 15, 20, 235)
    );

    overlay.setOutlineThickness(3.0f);

    overlay.setOutlineColor(
        sf::Color(255, 210, 70)
    );

    window.draw(overlay);

    drawText(
        window,
        font,
        "GAME OVER",
        48,
        500,
        270
    );

    drawText(
        window,
        font,
        "Winner: " + world.winner,
        27,
        500,
        350
    );

    drawText(
        window,
        font,
        "Final Score: " + to_string(world.winnerScore),
        23,
        500,
        400
    );

    drawText(
        window,
        font,
        "Press Q or close the window",
        18,
        500,
        450
    );
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    // --------------------------------------------------------
    // Windows Sockets
    // --------------------------------------------------------

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cerr << "WSAStartup failed.\n";
        return 1;
    }

    // --------------------------------------------------------
    // Server IP
    // --------------------------------------------------------

    cout << "Enter server IP (or press Enter for localhost): ";

    string serverIp;

    getline(cin, serverIp);

    if (serverIp.empty())
        serverIp = "127.0.0.1";

    // --------------------------------------------------------
    // Username
    // --------------------------------------------------------

    cout << "Enter your username: ";

    string username;

    getline(cin, username);

    if (username.empty())
        username = "Player";

    // --------------------------------------------------------
    // Create socket
    // --------------------------------------------------------

    sock = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (sock == INVALID_SOCKET)
    {
        cerr << "Could not create socket.\n";

        WSACleanup();

        return 1;
    }

    sockaddr_in server{};

    server.sin_family = AF_INET;

    server.sin_port = htons(54000);

    server.sin_addr.s_addr =
        inet_addr(serverIp.c_str());

    cout << "Connecting to "
        << serverIp
        << "...\n";

    if (
        connect(
            sock,
            reinterpret_cast<sockaddr*>(&server),
            sizeof(server)
        )
        == SOCKET_ERROR
        )
    {
        cerr << "Could not connect to server.\n";

        closesocket(sock);

        WSACleanup();

        return 1;
    }

    cout << "Connected!\n";

    // --------------------------------------------------------
    // Send username
    // --------------------------------------------------------

    send(
        sock,
        username.c_str(),
        static_cast<int>(username.size()),
        0
    );

    // --------------------------------------------------------
    // Start receiving thread
    // --------------------------------------------------------

    thread receiveThread(receiveLoop);

    // --------------------------------------------------------
    // Load font
    // --------------------------------------------------------

    sf::Font font;

    if (!font.openFromFile("assets/Inkfree.ttf"))
    {
        cerr << "Could not load assets/Inkfree.ttf\n";

        connected = false;

        shutdown(sock, SD_BOTH);

        receiveThread.join();

        closesocket(sock);

        WSACleanup();

        return 1;
    }

    // --------------------------------------------------------
    // Create SFML window
    // --------------------------------------------------------

    sf::RenderWindow window(
        sf::VideoMode({ 1200, 720 }),
        "Multiplayer Shooter"
    );

    window.setFramerateLimit(60);

    // --------------------------------------------------------
    // Input timing
    // --------------------------------------------------------

    auto lastMoveTime =
        chrono::steady_clock::now();

    auto lastShootTime =
        chrono::steady_clock::now();

    // --------------------------------------------------------
    // Main game loop
    // --------------------------------------------------------

    while (
        window.isOpen() &&
        connected
        )
    {
        // ----------------------------------------------------
        // Events
        // ----------------------------------------------------

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (
                const auto* keyPressed =
                event->getIf<sf::Event::KeyPressed>()
                )
            {
                if (
                    keyPressed->code ==
                    sf::Keyboard::Key::Q
                    )
                {
                    window.close();
                }

                if (
                    keyPressed->code ==
                    sf::Keyboard::Key::Space
                    )
                {
                    auto now =
                        chrono::steady_clock::now();

                    auto elapsed =
                        chrono::duration_cast<
                        chrono::milliseconds
                        >(now - lastShootTime).count();

                    if (elapsed >= 200)
                    {
                        sendCommand("SHOOT");

                        lastShootTime = now;
                    }
                }
            }
        }

        // ----------------------------------------------------
        // Continuous movement
        // ----------------------------------------------------

        auto now =
            chrono::steady_clock::now();

        auto moveElapsed =
            chrono::duration_cast<
            chrono::milliseconds
            >(now - lastMoveTime).count();

        if (moveElapsed >= 50)
        {
            if (
                sf::Keyboard::isKeyPressed(
                    sf::Keyboard::Key::A
                )
                )
            {
                sendCommand("MOVE_LEFT");
            }
            else if (
                sf::Keyboard::isKeyPressed(
                    sf::Keyboard::Key::D
                )
                )
            {
                sendCommand("MOVE_RIGHT");
            }
            else if (
                sf::Keyboard::isKeyPressed(
                    sf::Keyboard::Key::W
                )
                )
            {
                sendCommand("MOVE_UP");
            }
            else if (
                sf::Keyboard::isKeyPressed(
                    sf::Keyboard::Key::S
                )
                )
            {
                sendCommand("MOVE_DOWN");
            }

            lastMoveTime = now;
        }

        // ----------------------------------------------------
        // Copy current world safely
        // ----------------------------------------------------

        WorldState world;

        {
            lock_guard<mutex> lock(worldMutex);

            world = currentWorld;
        }

        // ----------------------------------------------------
        // Draw
        // ----------------------------------------------------

        window.clear(
            sf::Color(15, 17, 22)
        );

        drawHUD(
            window,
            font,
            world
        );

        drawGameArea(window);

        // Obstacles

        for (const auto& obstacle :
            world.obstacles)
        {
            drawObstacle(
                window,
                obstacle
            );
        }

        // PowerUps

        for (const auto& powerUp :
            world.powerUps)
        {
            drawPowerUp(
                window,
                powerUp
            );
        }

        // Bullets

        for (const auto& bullet :
            world.bullets)
        {
            drawBullet(
                window,
                bullet
            );
        }

        // Players

        int playerNumber = 1;

        for (const auto& player :
            world.players)
        {
            drawPlayer(
                window,
                font,
                player,
                playerNumber
            );

            playerNumber++;
        }

        // Game over

        if (world.gameOver)
        {
            drawGameOver(
                window,
                font,
                world
            );
        }

        window.display();
    }

    // --------------------------------------------------------
    // Shutdown
    // --------------------------------------------------------

    connected = false;

    shutdown(sock, SD_BOTH);

    if (receiveThread.joinable())
        receiveThread.join();

    closesocket(sock);

    WSACleanup();

    return 0;
}
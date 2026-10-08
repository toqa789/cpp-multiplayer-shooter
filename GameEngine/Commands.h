#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <string>
#include <memory>

class Player;


struct ICommand {
    virtual void execute(Player* player) = 0;
    virtual std::string name() const = 0;
    virtual ~ICommand() {}
};


struct MoveLeftCommand : public ICommand {
    void execute(Player* player) override;
    std::string name() const override { return "MOVE_LEFT"; }
};

struct MoveRightCommand : public ICommand {
    void execute(Player* player) override;
    std::string name() const override { return "MOVE_RIGHT"; }
};

struct MoveUpCommand : public ICommand {
    void execute(Player* player) override;
    std::string name() const override { return "MOVE_UP"; }
};

struct MoveDownCommand : public ICommand {
    void execute(Player* player) override;
    std::string name() const override { return "MOVE_DOWN"; }
};

struct ShootCommand : public ICommand {
    void execute(Player* player) override;
    std::string name() const override { return "SHOOT"; }
};

struct StopCommand : public ICommand {
    void execute(Player* player) override;
    std::string name() const override { return "STOP"; }
};


inline std::unique_ptr<ICommand> parseCommand(const std::string& cmd) {
    if (cmd == "MOVE_LEFT")  return std::make_unique<MoveLeftCommand>();
    if (cmd == "MOVE_RIGHT") return std::make_unique<MoveRightCommand>();
    if (cmd == "MOVE_UP")    return std::make_unique<MoveUpCommand>();
    if (cmd == "MOVE_DOWN")  return std::make_unique<MoveDownCommand>();
    if (cmd == "SHOOT")      return std::make_unique<ShootCommand>();
    if (cmd == "STOP")       return std::make_unique<StopCommand>();
    return nullptr;
}
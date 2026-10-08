#pragma once
#include "Commands.h"
#include "Player.h"

inline void MoveLeftCommand::execute(Player* player) { player->move(-15, 0); }
inline void MoveRightCommand::execute(Player* player) { player->move(15, 0); }
inline void MoveUpCommand::execute(Player* player) { player->move(0, -8); }
inline void MoveDownCommand::execute(Player* player) { player->move(0, 8); }
inline void StopCommand::execute(Player* player) { player->move(0, 0); player->setState("idle"); }


inline void ShootCommand::execute(Player* player) { player->attack(); }

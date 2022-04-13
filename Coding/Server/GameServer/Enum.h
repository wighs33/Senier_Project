#pragma once
#ifndef __ENUM_H__
#define __ENUM_H__


enum EVENT_TYPE { CL_BONEFIRE };
enum COMMAND {
	OP_RECV, OP_SEND, OP_ACCEPT, OP_NPC_MOVE, OP_NPC_ATTACK, OP_PLAYER_MOVE, OP_PLAYER_ATTACK, OP_PLAYER_RE, OP_PLAYER_HEAL
};
enum CL_STATE { ST_FREE, ST_ACCEPT, ST_INGAME };

enum COMBAT { COMBAT_DO, COMBAT_WIN, COMBAT_LOSE, COMBAT_RUN, COMBAT_END };

enum LEVEL { LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_END };
enum TYPE { TYPE_SWORD, TYPE_ARMOR, TYPE_END };
enum ITEM_STATE { STATE_EQUIP, STATE_UNEQUIP, STATE_END };

#endif // !__ENUM_H__

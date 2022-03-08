// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// winsock2 사용을 위해 아래 코멘트 추가
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <map>

#include "Final_Project.h"

#define	MAX_BUFFER		4096
#define SERVER_PORT		8000
#define SERVER_IP		"127.0.0.1"
#define MAX_CLIENTS		100

const int  MAX_NAME_SIZE = 20;
const int  MAX_CHAT_SIZE = 100;
const int BUF_SIZE = 255;
const static int MAX_BUFF_SIZE = 255;
// 소켓 통신 구조체

// 패킷 정보
enum CS_PacketType
{
	CS_PACKET_LOGIN = 1,
	CS_PACKET_MOVE,
	CS_PACKET_ATTACK,
	CS_PACKET_CHAT,
	CS_PACKET_TELEPORT
};

enum SC_PacketType
{
	SC_PACKET_LOGIN_OK = 1,
	SC_PACKET_MOVE,
	SC_PACKET_PUT_OBJECT,
	SC_PACKET_REMOVE_OBJECT,
	SC_PACKET_CHAT,
	SC_PACKET_LOGIN_FAIL,
	SC_PACKET_STATUS_CHANGE
};

// 패킷

#pragma pack (push, 1)
struct cs_packet_login {
	unsigned char size;
	char	type;
	char	name[MAX_NAME_SIZE];
};

struct cs_packet_move {
	unsigned char size;
	char	type;
	char	direction;			// 0 : up,  1: down, 2:left, 3:right
	int		move_time;
};

struct cs_packet_attack {
	unsigned char size;
	char	type;
};

struct cs_packet_chat {
	unsigned char size;
	char	type;
	char	message[MAX_CHAT_SIZE];
};

struct cs_packet_teleport {
	// 서버에서 장애물이 없는 랜덤 좌표로 텔레포트 시킨다.
	// 더미 클라이언트에서 동접 테스트용으로 사용.
	unsigned char size;
	char	type;
};

struct sc_packet_login_ok {
	unsigned char size;
	char type;
	int		id;
	short	x, y;
	short	level;
	short	hp, maxhp;
	int		exp;
};

struct sc_packet_move {
	unsigned char size;
	char type;
	int		id;
	short  x, y;
	int		move_time;
};

struct sc_packet_put_object {
	unsigned char size;
	char type;
	int id;
	short x, y;
	char object_type;
	char	name[MAX_NAME_SIZE];
};

struct sc_packet_remove_object {
	unsigned char size;
	char type;
	int id;
};

struct sc_packet_chat {
	unsigned char size;
	char type;
	int id;
	char message[MAX_CHAT_SIZE];
};

struct sc_packet_login_fail {
	unsigned char size;
	char type;
	int	 reason;		// 0: 중복 ID,  1:사용자 Full
};

struct sc_packet_status_change {
	unsigned char size;
	char type;
	short	level;
	short	hp, maxhp;
	int		exp;
};

static HANDLE g_hiocp;

enum OPTYPE { OP_SEND, OP_RECV, OP_DO_MOVE };

struct OverlappedEx {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	unsigned char IOCP_buf[MAX_BUFF_SIZE];
	OPTYPE event_type;
	int event_target;
};


class FINAL_PROJECT_API NetworkData
{
public:
};


#include "ClientSocket.h"
#include "MyPlayerController.h"


ClientSocket::~ClientSocket()
{
	closesocket(_socket);
	WSACleanup();
}

bool ClientSocket::Connect()
{
	MYLOG(Warning, TEXT("Connected begin!"));
	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		MYLOG(Warning, TEXT("Failed to start wsa"));

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(SERVER_PORT);

	int ret = connect(_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (ret == SOCKET_ERROR)
		return false;

	MYLOG(Warning, TEXT("Connected to Server!"));
	return true;
}

void ClientSocket::ProcessPacket(unsigned char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
	{
		sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
		_login_ok = true;

		// 캐릭터 정보
		cCharacter info;
		info.SessionId = packet->s_id;
		info.X = packet->x;
		info.Y = packet->y;
		info.Z = packet->z;
		info.Yaw = packet->yaw;
		info.iColor = packet->color;
		strcpy_s(info.userId, packet->id);
		my_s_id = packet->s_id;
		CharactersInfo.players[info.SessionId] = info;
		MyPlayerController->SetSessionId(info.SessionId);
		MyPlayerController->SetCharactersInfo(&CharactersInfo);
		MyPlayerController->SetInitInfo(info);

		// id, pw가 유효한 경우
		MyPlayerController->DeleteLoginUICreateReadyUI();	// Ready UI로 넘어가도록 하는 코드
		FString str = info.userId;
		//MYLOG(Warning, TEXT("[Recv put object] id : %s, sid : %d, location : (%f,%f,%f), yaw : %f"), *str, info.SessionId, info.X, info.Y, info.Z, info.Yaw);


		//MYLOG(Warning, TEXT("[Recv login ok] id : %d, location : (%f,%f,%f), yaw : %f"), info.SessionId, info.X, info.Y, info.Z, info.Yaw);

		break;
	}

	case SC_PACKET_LOGIN_FAIL:
	{
		sc_packet_login_fail* packet = reinterpret_cast<sc_packet_login_fail*>(ptr);

		//id, pw가 DB에 등록된 것과 일치하는지 체크
		//id가 유효하지 않은 경우 return
		switch (packet->reason)
		{
		case OVERLAP_AC:
		{
			MyPlayerController->loginInfoText = TEXT("account already connected");
			break;
		}
		case WORNG_ID:
		{
			MyPlayerController->loginInfoText = TEXT("Invalid ID");
			break;
		}
		case WORNG_PW:
		{
			MyPlayerController->loginInfoText = TEXT("Invalid PW");
			break;
		}
		case OVERLAP_ID:
		{
			MyPlayerController->loginInfoText = TEXT("Overlap_ID");
			break;
		}
		case CREATE_AC:
		{
			MyPlayerController->loginInfoText = TEXT("Account creation successful");
			break;
		}
		break;
		}
	}
	case SC_PACKET_READY:
	{
		sc_packet_ready* packet = reinterpret_cast<sc_packet_ready*>(ptr);
		int _s_id = packet->s_id;
		//MYLOG(Warning, TEXT("[player %d ] : READY"), packet->s_id);
		break;
	}

	case SC_PACKET_START:
	{
		// 게임시작
		MyPlayerController->StartSignal();
		break;
	}


	case SC_PACKET_PUT_OBJECT:
	{
		sc_packet_put_object* packet = reinterpret_cast<sc_packet_put_object*>(ptr);
		switch (packet->object_type)
		{

		case PLAYER:
		{
			auto info = make_shared<cCharacter>();
			info->SessionId = packet->s_id;
			info->iColor = packet->obj_id;
			info->X = packet->x;
			info->Y = packet->y;
			info->Z = packet->z;
			info->Yaw = packet->yaw;
			strcpy_s(info->userId, packet->name);
			info->myState = ST_ANIMAL;
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("put info-> userId :%s"), (TCHAR*)info->userId));
			MyPlayerController->SetNewCharacterInfo(info);
			FString str = info->userId;
			//MYLOG(Warning, TEXT("[Recv put object] id : %s, sid : %d, location : (%f,%f,%f), yaw : %f"),*str ,info->SessionId, info->X, info->Y, info->Z, info->Yaw);

			break;
		}
		case ITEM_BOX:
		{
			break;
		}
		case TONARDO:
		{
			auto info = make_shared<cCharacter>();
			info->SessionId = packet->s_id;
			info->X = packet->x;
			info->Y = packet->y;
			info->Z = packet->z;
			info->Yaw = packet->yaw;
			info->myState = ST_TORNADO;
			MYLOG(Warning, TEXT("recv TONARDO"));
			MyPlayerController->SetNewTornadoInfo(info);
			//MYLOG(Warning, TEXT("[Recv put object] id : %d, location : (%f,%f,%f), yaw : %f"), info->SessionId, info->X, info->Y, info->Z, info->Yaw);

			break;
		}
		case SUPPLYBOX:
		{

			MyPlayerController->SpawnSupplyBox(packet->x, packet->y, packet->z);

			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("spbox :%f, %f"), packet->x, packet->y));

			break;
		}
		default:
			break;
		}
		break;
	}

	case SC_PACKET_MOVE:
	{
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(ptr);

		CharactersInfo.players[packet->sessionID].X = packet->x;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].Y = packet->y;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].Z = packet->z;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].Yaw = packet->yaw;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].VX = packet->vx;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].VY = packet->vy;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].VZ = packet->vz;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].direction = packet->direction;		// 캐릭터 정보

		//MYLOG(Warning, TEXT("[Recv move] id: %d, location: (%f,%f,%f), yaw: %f, velocity: (%f,%f,%f), dir: %f"), packet->sessionID, packet->x, packet->y, packet->z, packet->yaw, packet->vx, packet->vy, packet->vz, packet->direction);
		break;
	}

	case SC_PACKET_THROW_SNOW:
	{

		cs_packet_throw_snow* packet = reinterpret_cast<cs_packet_throw_snow*>(ptr);
		CharactersInfo.players[packet->s_id].fSpeed = packet->speed;		// 발사 속도
		CharactersInfo.players[packet->s_id].fCYaw = packet->yaw;		// yaw
		CharactersInfo.players[packet->s_id].fCPitch = packet->pitch;		// pitch
		CharactersInfo.players[packet->s_id].fCRoll = packet->roll;		// roll

		switch (packet->bullet)
		{
		case BULLET_SNOWBALL:
		{
			CharactersInfo.players[packet->s_id].fSBallX = packet->ball_x;		// 눈덩이 위치
			CharactersInfo.players[packet->s_id].fSBallY = packet->ball_y;		// 눈덩이 위치
			CharactersInfo.players[packet->s_id].fSBallZ = packet->ball_z;		// 눈덩이 위치

			MyPlayerController->SetAttack(packet->s_id, END_SNOWBALL);
			break;
		}
		case BULLET_ICEBALL:
		{
			CharactersInfo.players[packet->s_id].fIBallX = packet->ball_x;		// 얼음 위치
			CharactersInfo.players[packet->s_id].fIBallY = packet->ball_y;		// 얼음 위치
			CharactersInfo.players[packet->s_id].fIBallZ = packet->ball_z;		// 얼음 위치

			MyPlayerController->SetAttack(packet->s_id, END_ICEBALL);
			break;
		}
		default:
			break;
		}

		break;
	}
	case SC_PACKET_CANCEL_SNOW:
	{

		cs_packet_cancel_snow* packet = reinterpret_cast<cs_packet_cancel_snow*>(ptr);

		switch (packet->bullet)
		{
		case BULLET_SNOWBALL:
		{
			MyPlayerController->SetAttack(packet->s_id, CANCEL_SNOWBALL);
			break;
		}
		case BULLET_ICEBALL:
		{
			MyPlayerController->SetAttack(packet->s_id, CANCEL_ICEBALL);
			break;
		}
		default:
			break;
		}
		break;
	}
	case SC_PACKET_GUNFIRE:
	{
		cs_packet_fire* packet = reinterpret_cast<cs_packet_fire*>(ptr);
		for (int i = 0; i < MAX_BULLET_RANG; ++i)
			CharactersInfo.players[packet->s_id].iRandBulletArr[i] = packet->rand_int[i];
		CharactersInfo.players[packet->s_id].Pitch = packet->pitch;		// 카메라 pitch
		MyPlayerController->SetAttack(packet->s_id, END_SHOTGUN);
		break;
	}
	case SC_PACKET_HP:
	{
		sc_packet_hp_change* packet = reinterpret_cast<sc_packet_hp_change*>(ptr);

		CharactersInfo.players[packet->s_id].HealthValue = packet->hp;

		//MYLOG(Warning, TEXT("[Recv hp change] id : %d, hp : %d"), packet->s_id, packet->hp);

		break;

	}

	case SC_PACKET_REMOVE_OBJECT:
	{
		cs_packet_throw_snow* packet = reinterpret_cast<cs_packet_throw_snow*>(ptr);

		//MYLOG(Warning, TEXT("[Recv remove object] id : %d"), packet->s_id);

		break;
	}

	case SC_PACKET_STATUS_CHANGE:
	{
		sc_packet_status_change* packet = reinterpret_cast<sc_packet_status_change*>(ptr);
		//MYLOG(Warning, TEXT("[Recv status change] id : %d, state : %d"), packet->s_id, packet->state);
		if (ST_SNOWMAN == packet->state) {
			
			CharactersInfo.players[packet->s_id].myState = ST_SNOWMAN;
			//MYLOG(Warning, TEXT("snowMAN !!! [ %d ] "), CharactersInfo.players[packet->s_id].HealthValue);
		}
		else if (ST_ANIMAL == packet->state) {
			CharactersInfo.players[packet->s_id].myState = ST_ANIMAL;
		}
		break;
	}

	case SC_PACKET_GET_ITEM:
	{
		cs_packet_get_item* packet = reinterpret_cast<cs_packet_get_item*>(ptr);

		switch (packet->item_type)
		{

		case ITEM_BAG:
		{
		    MyPlayerController->GetItem(packet->s_id, ITEM_BAG);
			MyPlayerController->SetDestroyitembox(packet->destroy_obj_id);
			break;
		}
		case ITEM_UMB:
		{
			if (MyPlayerController->iSessionId == packet->s_id)
				MyPlayerController->GetItem(packet->s_id, ITEM_UMB);
			MyPlayerController->SetDestroyitembox(packet->destroy_obj_id);

			break;
		}
		case ITEM_MAT:
		{
			if (MyPlayerController->iSessionId == packet->s_id)
				MyPlayerController->GetItem(packet->s_id, ITEM_MAT);
			//CharactersInfo.players[packet->s_id].iCurrentMatchCount++;
			MyPlayerController->SetDestroyitembox(packet->destroy_obj_id);

			break;
		}
		case ITEM_SNOW:
		{
			CharactersInfo.players[packet->s_id].iCurrentSnowCount = packet->current_bullet;
			MyPlayerController->SetDestroySnowdritt(packet->destroy_obj_id);
			break;
		}
		case ITEM_ICE:
		{
			CharactersInfo.players[packet->s_id].iCurrentIceCount = packet->current_bullet;
			MyPlayerController->SetDestroyIcedritt(packet->destroy_obj_id);
			break;
		}
		case ITEM_JET:
		{
			if (MyPlayerController->iSessionId != packet->s_id)
				MyPlayerController->SetItem(packet->s_id, ITEM_JET, true);
			break;
		}
		case ITEM_SPBOX:
		{
			CharactersInfo.players[packet->s_id].iCurrentSnowCount = packet->current_bullet;
			CharactersInfo.players[packet->s_id].iCurrentIceCount = packet->current_bullet;
			MyPlayerController->GetItem(packet->s_id, ITEM_SPBOX);
			MyPlayerController->SetDestroySpBox(packet->destroy_obj_id);
			break;
		}
		default:
			break;
		}

		//MYLOG(Warning, TEXT("[Recv item] id : %d, item : %d"), packet->s_id, packet->item_no);
		break;
	}
	case SC_PACKET_IS_BONE:
	{
		MyPlayerController->GetBone();
		break;
	}
	case SC_PACKET_LOGOUT:
	{
		cs_packet_logout* packet = reinterpret_cast<cs_packet_logout*>(ptr);
		int del_s_id = packet->s_id;
		//MYLOG(Warning, TEXT("player%d logout "), del_s_id);
		MyPlayerController->SetDestroyPlayer(del_s_id);

		break;
	}

	case SC_PACKET_END:
	{
		sc_packet_game_end* packet = reinterpret_cast<sc_packet_game_end*>(ptr);
		int end_s_id = packet->s_id;
		//MYLOG(Warning, TEXT("player%d victory "), end_s_id);
		MyPlayerController->SetGameEnd(end_s_id);

		break;
	}
	case SC_PACKET_OPEN_BOX:
	{
		cs_packet_open_box* packet = reinterpret_cast<cs_packet_open_box*>(ptr);
		int open_obj_id = packet->open_obj_id;
		MYLOG(Warning, TEXT("recv open box "));
		MyPlayerController->SetOpenItembox(open_obj_id);

		break;
	}

	case SC_PACKET_ATTACK:
	{
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(ptr);
		switch (packet->bullet)
		{
		case BULLET_SNOWBALL:
		{
			MyPlayerController->SetAttack(packet->s_id, ATTACK_SNOWBALL);
			break;
		}
		case BULLET_ICEBALL:
		{
			MyPlayerController->SetAttack(packet->s_id, ATTACK_ICEBALL);
			break;
		}
		default:
			break;
		}

		break;
	}
	case SC_PACKET_GUNATTACK:
	{
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(ptr);
		MyPlayerController->SetAttack(packet->s_id, ATTACK_SHOTGUN);
		break;
	}
	case SC_PACKET_UMB:
	{
		cs_packet_umb* packet = reinterpret_cast<cs_packet_umb*>(ptr);
		MyPlayerController->SetItem(packet->s_id, ITEM_UMB, packet->end);
		break;
	}
	case SC_PACKET_PLAYER_COUNT:
	{
		sc_packet_player_count* packet = reinterpret_cast<sc_packet_player_count*>(ptr);
		MyPlayerController->SetCnt(packet->bear, packet->snowman);
		break;
	}
	case SC_PACKET_NPC_MOVE:
	{
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(ptr);

		CharactersInfo.players[packet->sessionID].X = packet->x;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].Y = packet->y;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].Z = packet->z;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].Yaw = packet->yaw;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].VX = packet->vx;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].VY = packet->vy;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].VZ = packet->vz;		// 캐릭터 정보
		CharactersInfo.players[packet->sessionID].direction = packet->direction;		// 캐릭터 정보
		break;
	} 
	case SC_PACKET_KILL_LOG:
	{
		sc_packet_kill_log* packet = reinterpret_cast<sc_packet_kill_log*>(ptr);
		MyPlayerController->CallDelegateUpdateKillLog(packet->attacker, packet->victim, packet->cause);

		break;
	}
	case SC_PACKET_FREEZE:
	{
		cs_packet_freeze* packet = reinterpret_cast<cs_packet_freeze*>(ptr);
		MyPlayerController->SetFreeze(packet->s_id, packet->boddyparts);
		break;
	}
	}
}

void ClientSocket::SetPlayerController(AMyPlayerController* pPlayerController)
{
	// 플레이어 컨트롤러 세팅
	if (pPlayerController)
	{
		MyPlayerController = pPlayerController;
	}
}

void ClientSocket::Send_LoginPacket(char* send_id, char* send_pw)
{
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGIN;
	strcpy_s(packet.id, send_id);
	strcpy_s(packet.pw, send_pw);

	auto player_ = Cast<AMyCharacter>(UGameplayStatics::GetPlayerCharacter(MyPlayerController, 0));
	if (!player_) return;
	auto location_ = player_->GetActorLocation();

	packet.z = location_.Z;
	size_t sent = 0;


	//MYLOG(Warning, TEXT("[Send login] z : %f"), packet.z);
	SendPacket(&packet);
	


};

void ClientSocket::Send_Create_ID_Packet(char* send_id, char* send_pw)
{
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_ACCOUNT;
	strcpy_s(packet.id, send_id);
	strcpy_s(packet.pw, send_pw);
	size_t sent = 0;
	//MYLOG(Warning, TEXT("[Send login] z : %f"), packet.z);
	SendPacket(&packet);

};

void ClientSocket::Send_ReadyPacket()
{
	cs_packet_ready packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_READY;
	SendPacket(&packet);

}


void ClientSocket::Send_StatusPacket(int _state, int s_id) {
	//CharactersInfo.players[iMy_s_id].myState = _state;
	sc_packet_status_change packet;
	packet.size = sizeof(packet);
	packet.s_id = s_id;
	packet.type = CS_PACKET_STATUS_CHANGE;
	packet.state = _state;

	//MYLOG(Warning, TEXT("[Send status] status : %d"), _state);
	SendPacket(&packet);
};

void ClientSocket::Send_DamagePacket(int attacker, int bullet) {
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Send_DamagePacket")));
	cs_packet_damage packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_DAMAGE;
	packet.attacker = attacker;
	packet.bullet = bullet;
	SendPacket(&packet);
};

void ClientSocket::Send_AttackPacket(int s_id, int bullet) {
	cs_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_ATTACK;
	packet.s_id = s_id;
	packet.bullet = bullet;
	//MYLOG(Warning, TEXT("[Send damage]"));
	SendPacket(&packet);
};

void ClientSocket::Send_GunAttackPacket(int s_id) {
	cs_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_GUNATTACK;
	packet.s_id = s_id;

	//MYLOG(Warning, TEXT("[Send damage]"));
	SendPacket(&packet);
};

void ClientSocket::Send_MatchPacket() {
	cs_packet_match packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_MATCH;

	//MYLOG(Warning, TEXT("[Send damage]"));
	SendPacket(&packet);
};

void ClientSocket::Send_UmbPacket(bool umb_use) {
	cs_packet_umb packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_UMB;
	packet.s_id = MyPlayerController->iSessionId;
	packet.end = umb_use;
	//MYLOG(Warning, TEXT("[Send damage]"));
	SendPacket(&packet);
};

void ClientSocket::Send_FreezePacket(int s_id, int bobyparts) {
	cs_packet_freeze packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_FREEZE;
	packet.s_id = s_id;
	packet.boddyparts = bobyparts;
	SendPacket(&packet);
};

void ClientSocket::Send_TelePortPacket(int point_num) {
	cs_packet_teleport packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_TELEPORT;
	packet.Point = point_num;
	SendPacket(&packet);
};
void ClientSocket::Send_MovePacket(int s_id, FVector MyLocation, float yaw, FVector MyVelocity, float dir)
{
	if (_login_ok) {
		cs_packet_move packet;
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_MOVE;
		packet.sessionID = s_id;
		packet.x = MyLocation.X;
		packet.y = MyLocation.Y;
		packet.z = MyLocation.Z;
		packet.yaw = yaw;
		packet.vx = MyVelocity.X;
		packet.vy = MyVelocity.Y;
		packet.vz = MyVelocity.Z;
		packet.direction = dir;

		//MYLOG(Warning, TEXT("[Send move] id: %d, location: (%f,%f,%f), yaw: %f, velocity: (%f,%f,%f), dir: %f"), s_id, packet.x, packet.y, packet.z, packet.yaw, packet.vx, packet.vy, packet.vz, dir);
		SendPacket(&packet);
	}
};


void ClientSocket::Send_Throw_Packet(int s_id, FVector BallLocation, FRotator MyRotation, int bullet, float speed)
{

	cs_packet_throw_snow packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_THROW_SNOW;
	packet.s_id = s_id;
	packet.bullet = bullet;
	packet.ball_x = BallLocation.X;
	packet.ball_y = BallLocation.Y;
	packet.ball_z = BallLocation.Z;
	packet.yaw = MyRotation.Yaw;
	packet.pitch = MyRotation.Pitch;
	packet.roll = MyRotation.Roll;
	packet.speed = speed;
	size_t sent = 0;
	//MYLOG(Warning, TEXT("[Send throw snow] id: %d, loc: (%f, %f, %f), dir: (%f, %f, %f)"), s_id, packet.x, packet.y, packet.z, packet.dx, packet.dy, packet.dz);
	SendPacket(&packet);
};

void ClientSocket::Send_Cancel_Packet(int s_id, int bullet)
{

	cs_packet_cancel_snow packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CANCEL_SNOW;
	packet.s_id = s_id;
	packet.bullet = bullet;
	SendPacket(&packet);
};

void ClientSocket::Send_GunFire_Packet(int s_id, FVector MyLocation, FRotator MyRotation)
{
	//MYLOG(Warning, TEXT("Send_GunFire_Packet"));
	cs_packet_fire packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_GUNFIRE;
	packet.s_id = s_id;
	packet.pitch = MyRotation.Pitch;
	size_t sent = 0;
	SendPacket(&packet);
};


void ClientSocket::Send_ItemPacket(int item_type, int destroy_obj_id)
{
	cs_packet_get_item packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_GET_ITEM;
	packet.s_id = MyPlayerController->iSessionId;
	packet.item_type = item_type;
	packet.destroy_obj_id = destroy_obj_id;
	size_t sent = 0;

	//MYLOG(Warning, TEXT("[Send item] id : %d, objId : %d, item : %d"), packet.s_id, destroy_obj_id, item_type);
	SendPacket(&packet);
};

void ClientSocket::Send_OpenBoxPacket(int open_box_id)
{
	cs_packet_open_box packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_OPEN_BOX;
	packet.open_obj_id = open_box_id;
	SendPacket(&packet);
	MYLOG(Warning, TEXT("send open box "));

};

void ClientSocket::SendPutObjPacket(char cObjType, int iObjId, FVector ObjLocation, float fYaw)
{
	sc_packet_put_object packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_PUT_OBJECT;
	packet.s_id = MyPlayerController->iSessionId;
	packet.obj_id = iObjId;
	packet.object_type = cObjType;
	packet.x = ObjLocation.X;
	packet.y = ObjLocation.Y;
	packet.z = ObjLocation.Z;
	packet.yaw = fYaw;
	size_t sent = 0;
	//MYLOG(Warning, TEXT("[Send item] id : %d, objId : %d, item : %d"), packet.s_id, destroy_obj_id, item_type);
	SendPacket(&packet);
};


void ClientSocket::Send_ChatPacket(int cheat_num)
{
	cs_packet_chat packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_CHAT;
	packet.s_id = MyPlayerController->iSessionId;
	packet.cheat_type = cheat_num;
	size_t sent = 0;
	SendPacket(&packet);
};

void ClientSocket::Send_LogoutPacket(const int& s_id)
{
	cs_packet_logout packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGOUT;
	packet.s_id = MyPlayerController->iSessionId;
	//MYLOG(Warning, TEXT("[Send Logout] id : %d"), packet.s_id);
	SendPacket(&packet);
}


void ClientSocket::CloseSocket()
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER 로 설정
										 //강제 종료 시킨다. 데이터 손실이 있을 수 있음

	stLinger.l_onoff = true;
	//_socket 소켓의 데이터 송수신을 모두 중단
	shutdown(_socket, SD_BOTH);
	//소켓 옵션을 설정
	setsockopt(_socket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
	//소켓 연결을 종료
	closesocket(_socket);

	_socket = INVALID_SOCKET;

	WSACleanup();
	MYLOG(Warning, TEXT("CloseSocket"));

}

bool ClientSocket::Init()
{
	return true;
}

void ClientSocket::process_data(unsigned char* net_buf, size_t io_byte)
{
	//MYLOG(Warning, TEXT("process_data!"));

	unsigned char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static unsigned char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
	if (io_byte == 0)
		_prev_size = 0;
}

uint32 ClientSocket::Run()
{
	// 초기 init 과정을 기다림
	FPlatformProcess::Sleep(0.03);

	//Connect();
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(_socket), h_iocp, 0, 0);

	RecvPacket();

	//Send_LoginPacket();

	SleepEx(0, true);

	// recv while loop 시작
	// StopTaskCounter 클래스 변수를 사용해 Thread Safety하게 해줌
	while (StopTaskCounter.GetValue() == 0 && MyPlayerController != nullptr)
	{
		DWORD num_byte;
		LONG64 iocp_key;
		WSAOVERLAPPED* p_over;

		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_byte, (PULONG_PTR)&iocp_key, &p_over, INFINITE);

		Overlap* exp_over = reinterpret_cast<Overlap*>(p_over);

		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			if (exp_over->_op == OP_SEND)
				delete exp_over;
			continue;
		}

		switch (exp_over->_op) {
		case OP_RECV: {
			if (num_byte == 0) {
				//Disconnect();
				continue;
			}
			int remain_data = num_byte + _prev_size;
			unsigned char* packet_start = exp_over->_net_buf;
			int packet_size = packet_start[0];
			while (packet_size <= remain_data) {
				ProcessPacket(packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				_prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}

			RecvPacket();
			SleepEx(0, true);
			break;
		}
		case OP_SEND: {
			if (num_byte != exp_over->_wsa_buf.len) {
				//Disconnect();
			}
			delete exp_over;
			break;
		}

		}

	}
	return 0;
}

void ClientSocket::Stop()
{
	// thread safety 변수를 조작해 while loop 가 돌지 못하게 함
	StopTaskCounter.Increment();
}

void ClientSocket::Exit()
{
}


bool ClientSocket::StartListen()
{
	// 스레드 시작
	if (Thread != nullptr) return false;
	Thread = FRunnableThread::Create(this, TEXT("ClientSocket"), 0, TPri_BelowNormal);
	return (Thread != nullptr);
}

void ClientSocket::StopListen()
{
	// 스레드 종료
	Stop();
	Thread->WaitForCompletion();
	Thread->Kill();
	delete Thread;
	Thread = nullptr;
	StopTaskCounter.Reset();
}

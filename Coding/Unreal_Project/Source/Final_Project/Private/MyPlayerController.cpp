// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"
#include "MyGameModeBase.h"
#include "ClientSocket.h"
#include "MyAnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Debug.h"
#include "Snowdrift.h"
#include "Itembox.h"

#pragma comment(lib, "ws2_32.lib")

ClientSocket* g_socket = nullptr;

void CALLBACK recv_callback(DWORD err, DWORD num_byte, LPWSAOVERLAPPED recv_over, DWORD recv_flag)
{
	//MYLOG(Warning, TEXT("recv_callback"));
	if (num_byte == 0) {
		recv_flag = 0;
		/*ZeroMemory(&mySocket->_recv_over._wsa_over, sizeof(mySocket->_recv_over._wsa_over));
		mySocket->_recv_over._wsa_buf.buf = reinterpret_cast<char*>(mySocket->_recv_over._net_buf + mySocket->_prev_size);
		mySocket->_recv_over._wsa_buf.len = sizeof(mySocket->_recv_over._net_buf) - mySocket->_prev_size;
		WSARecv(mySocket->_socket, &mySocket->_recv_over._wsa_buf, 1, 0, &recv_flag, &mySocket->_recv_over._wsa_over, recv_callback);*/
		MYLOG(Warning, TEXT("recv_error"));

		return;
	}
	unsigned char* packet_start = g_socket->_recv_over._net_buf;
	int packet_size = packet_start[0];
	g_socket->process_data(g_socket->_recv_over._net_buf, num_byte);
	recv_flag = 0;
	ZeroMemory(&g_socket->_recv_over._wsa_over, sizeof(g_socket->_recv_over._wsa_over));
	g_socket->_recv_over._wsa_buf.buf = reinterpret_cast<char*>(g_socket->_recv_over._net_buf + g_socket->_prev_size);
	g_socket->_recv_over._wsa_buf.len = sizeof(g_socket->_recv_over._net_buf) - g_socket->_prev_size;
	WSARecv(g_socket->_socket, &g_socket->_recv_over._wsa_buf, 1, 0, &recv_flag, &g_socket->_recv_over._wsa_over, recv_callback);
	SleepEx(0, true);
}

AMyPlayerController::AMyPlayerController()
{
	
	//bNewPlayerEntered = false;
	bInitPlayerSetting = false;
	bSetStart.store(false);
	victory_player.store(-1);
	bInGame = false;
	PrimaryActorTick.bCanEverTick = true;
	
	static ConstructorHelpers::FClassFinder<UUserWidget> READY_UI(TEXT("/Game/Blueprints/ReadyUI.ReadyUI_C"));
	if (READY_UI.Succeeded() && (READY_UI.Class != nullptr))
	{
		readyUIClass = READY_UI.Class;
	}
	static ConstructorHelpers::FClassFinder<UUserWidget> CHARACTER_UI(TEXT("/Game/Blueprints/CharacterUI.CharacterUI_C"));
	if (CHARACTER_UI.Succeeded() && (CHARACTER_UI.Class != nullptr))
	{
		characterUIClass = CHARACTER_UI.Class;
	}
	static ConstructorHelpers::FClassFinder<UUserWidget> GAMERESULT_UI(TEXT("/Game/Blueprints/GameOverUI.GameOverUI_C"));
	if (GAMERESULT_UI.Succeeded() && (GAMERESULT_UI.Class != nullptr))
	{
		gameResultUIClass = GAMERESULT_UI.Class;
	}
	bIsReady = false;
	
}

void AMyPlayerController::OnPossess(APawn* pawn_)
{
	Super::OnPossess(pawn_);

	//mySocket->StartListen();
	localPlayerCharacter = Cast<AMyCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	LoadReadyUI();	// readyUI 띄우고 게임에 대한 입력 x, UI에 대한 입력만 받음
	
	FPlatformProcess::Sleep(0);
}

void AMyPlayerController::BeginPlay()
{
	MYLOG(Warning, TEXT("BeginPlay!"));

	// 실행시 클릭없이 바로 조작
	//FInputModeGameOnly InputMode;
	//SetInputMode(InputMode);
	//mySocket = ClientSocket::GetSingleton();
	//mySocket->SetPlayerController(this);
	//mySocket = ClientSocket::GetSingleton();
	//mySocket->SetPlayerController(this);
	//mySocket = mySocket;
	//mySocket->Connect();
	//mySocket->Connect();
	/*DWORD recv_flag = 0;
	ZeroMemory(&mySocket->_recv_over._wsa_over, sizeof(mySocket->_recv_over._wsa_over));
	mySocket->_recv_over._wsa_buf.buf = reinterpret_cast<char*>(mySocket->_recv_over._net_buf + mySocket->_prev_size);
	mySocket->_recv_over._wsa_buf.len = sizeof(mySocket->_recv_over._net_buf) - mySocket->_prev_size;
	WSARecv(mySocket->_socket, &mySocket->_recv_over._wsa_buf, 1, 0, &recv_flag, &mySocket->_recv_over._wsa_over, recv_callback);
	mySocket->Send_LoginPacket();*/
	g_socket = ClientSocket::GetSingleton();
	g_socket->SetPlayerController(this);
	mySocket = g_socket;
	g_socket->Connect();
	DWORD recv_flag = 0;
	ZeroMemory(&g_socket->_recv_over._wsa_over, sizeof(g_socket->_recv_over._wsa_over));
	g_socket->_recv_over._wsa_buf.buf = reinterpret_cast<char*>(g_socket->_recv_over._net_buf + g_socket->_prev_size);
	g_socket->_recv_over._wsa_buf.len = sizeof(g_socket->_recv_over._net_buf) - g_socket->_prev_size;
	WSARecv(g_socket->_socket, &g_socket->_recv_over._wsa_buf, 1, 0, &recv_flag, &g_socket->_recv_over._wsa_over, recv_callback);
	g_socket->Send_LoginPacket();
	SleepEx(0, true);
	
	//mySocket->StartListen();
	SleepEx(0, true);
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//MYLOG(Warning, TEXT("EndPlay!"));
	//mySocket->Send_LogoutPacket(iSessionId);
	//mySocket->CloseSocket();
	//mySocket->StopListen();
	
	// 델리게이트 해제
	FuncUpdateHP.Clear();
	FuncUpdateCurrentSnowballCount.Clear();
	FuncUpdateCurrentMatchCount.Clear();
	FuncUpdateMaxSnowballAndMatchCount.Clear();
	FuncUpdateHasUmbrella.Clear();
	FuncUpdateHasBag.Clear();
	FuncUpdateIsFarmingSnowdrift.Clear();
	FuncUpdateSnowdriftFarmDuration.Clear();
	FuncUpdateSelectedItem.Clear();
	FuncUpdateGameResult.Clear();
}

void AMyPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	////확인용
	//TArray<AActor*> Itemboxes;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItembox::StaticClass(), Itemboxes);

	//for (auto ib : Itemboxes)
	//{
	//	AItembox* itembox = Cast<AItembox>(ib);

	//	MYLOG(Warning, TEXT("id : %d"), itembox->GetId());
	//}


	//플레이어 초기설정
	if (bInitPlayerSetting)
		InitPlayerSetting();

	if (newPlayers.TryPop(newplayer))
		UpdateNewPlayer();

	if (bSetStart)
		StartGame();
	if (victory_player != -1)
		LoadGameResultUI(victory_player);

	// 월드 동기화
	UpdateWorldInfo();

	SendPlayerInfo(COMMAND_MOVE);

	SleepEx(0, true);

	//UpdateRotation();
}

void AMyPlayerController::SetInitInfo(const cCharacter& me)
{
	initInfo = me;
	bInitPlayerSetting = true;
}

void AMyPlayerController::SetNewCharacterInfo(shared_ptr<cCharacter> NewPlayer_)
{
	if (NewPlayer_ != nullptr)
	{
		//bNewPlayerEntered = true;
		newPlayers.Push(NewPlayer_);
	}
}

void AMyPlayerController::SetNewBall(const int s_id)
{
	UWorld* World = GetWorld();
	newBalls.Push(s_id);
}

void AMyPlayerController::SetDestroyPlayer(const int del_sid)
{
	UWorld* World = GetWorld();
	destory_player.Push(del_sid);
}

void AMyPlayerController::SetDestroySnowdritt(const int obj_id)
{
	UWorld* World = GetWorld();
	destory_snowdrift.Push(obj_id);
}

void AMyPlayerController::SetOpenItembox(const int obj_id)
{
	UWorld* World = GetWorld();
	open_itembox.Push(obj_id);
}

void AMyPlayerController::SetGameEnd(const int target_id)
{
	UWorld* World = GetWorld();
	victory_player.store(target_id);
}

void AMyPlayerController::SetDestroyitembox(const int obj_id)
{
	UWorld* World = GetWorld();
	destory_itembox.Push(obj_id);
}

void AMyPlayerController::InitPlayerSetting()
{
	if (!localPlayerCharacter) return;
	localPlayerCharacter->SetActorLocation(FVector(initInfo.X, initInfo.Y, initInfo.Z));
	localPlayerCharacter->iSessionId = initInfo.SessionId;

	//컨트롤러의 회전
	SetControlRotation(FRotator(0.0f, initInfo.Yaw, 0.0f));

	localPlayerCharacter->SetCharacterMaterial(iSessionId);

	bInitPlayerSetting = false;
}

bool AMyPlayerController::UpdateWorldInfo()
{
	UWorld* const world = GetWorld();
	if (world == nullptr)					return false;
	if (charactersInfo == nullptr)	return false;


	int id_ = -1;
	while (newBalls.TryPop(id_))
	{
		charactersInfo->players[id_].canAttack = true;
		id_ = -1;
	}


	id_ = -1;
	while (destory_snowdrift.TryPop(id_))
	{
		TArray<AActor*> Snowdrifts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASnowdrift::StaticClass(), Snowdrifts);

		for (auto sd : Snowdrifts)
		{
			ASnowdrift* snowdrift = Cast<ASnowdrift>(sd);

			if (snowdrift->GetId() == id_)
			{
				snowdrift->Destroy();
				snowdrift = nullptr;
			}
		}
		id_ = -1;
	}

	id_ = -1;
	while (open_itembox.TryPop(id_))
	{
		TArray<AActor*> ItemBox;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItembox::StaticClass(), ItemBox);

		for (auto sd : ItemBox)
		{
			AItembox* itembox = Cast<AItembox>(sd);
			//if (!itembox) continue;
			if (itembox->GetId() == id_)
			{
				itembox->SetItemboxState(ItemboxState::Opening);
			}
		}
		id_ = -1;
	}

	id_ = -1;
	while (destory_itembox.TryPop(id_))
	{
		TArray<AActor*> ItemBox;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItembox::StaticClass(), ItemBox);

		for (auto sd : ItemBox)
		{
			AItembox* itembox = Cast<AItembox>(sd);

			if (itembox->GetId() == id_)
			{
				itembox->DeleteItem();

			}
		}
		id_ = -1;
	}
	
	TArray<AActor*> delCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyCharacter::StaticClass(), delCharacters);

	id_ = -1;
	while (destory_player.TryPop(id_))
	{

		for (auto sd : delCharacters)
		{
			AMyCharacter* Del_Player = Cast<AMyCharacter>(sd);

			if (Del_Player->iSessionId == id_)
			{
				charactersInfo->players.erase(Del_Player->iSessionId);
				Del_Player->Destroy();
				Del_Player = nullptr;
				
			}
		}
		id_ = -1;
	}



	// 플레이어자신 체력, 사망업데이트
	UpdatePlayerInfo(charactersInfo->players[iSessionId]);

	if (charactersInfo->players.size() == 1)
	{
		//MYLOG(Warning, TEXT("Only one player"));
		return false;
	}

	// 스폰캐릭터들배열 하나 생성하고 월드에 있는 캐릭터들을 배열에 넣어주기
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyCharacter::StaticClass(), SpawnedCharacters);
	
	
	for (auto& Character_ : SpawnedCharacters)
	{
		//자신포함 모든플레이어
		AMyCharacter* player_ = Cast<AMyCharacter>(Character_);

		cCharacter* info = &charactersInfo->players[player_->iSessionId];
		if (!info->IsAlive) continue;

		if (info->canAttack) { 
			player_->SnowAttack();
			info->canAttack = false;
			info->current_snow_count--;
		}

		//타플레이어 구별
		if (!player_ || player_->iSessionId == -1 || player_->iSessionId == iSessionId)
		{
			continue;
		}


		//if (player_->iCurrentHP != info->HealthValue)
		//{
		//	MYLOG(Warning, TEXT("other player damaged."));
		//	//// 피격 파티클 소환
		//	//FTransform transform(OtherPlayer->GetActorLocation());
		//	//UGameplayStatics::SpawnEmitterAtLocation(
		//	//	world, HitEmiiter, transform, true
		//	//);
		//	//// 피격 애니메이션 플레이
		//	//player_->PlayDamagedAnimation();
		//	//player_->iCurrentHP = info->HealthValue;
		//}

		//// 공격중일때 타격 애니메이션 플레이
		//if (info->IsAttacking)
		//{
		//	UE_LOG(LogClass, Log, TEXT("other player hit."));
		//	OtherPlayer->PlayHitAnimation();
		//}

		FVector CharacterLocation;
		CharacterLocation.X = info->X;
		CharacterLocation.Y = info->Y;
		CharacterLocation.Z = info->Z;

		FRotator CharacterRotation;
		CharacterRotation.Yaw = info->Yaw;
		CharacterRotation.Pitch = 0.0f;
		CharacterRotation.Roll = 0.0f;

		FVector CharacterVelocity;
		CharacterVelocity.X = info->VX;
		CharacterVelocity.Y = info->VY;
		CharacterVelocity.Z = info->VZ;

		player_->AddMovementInput(CharacterVelocity);
		player_->SetActorRotation(CharacterRotation);
		player_->SetActorLocation(CharacterLocation);
		player_->GetAnim()->SetDirection(info->direction);
		//눈사람 변화
		if (!player_->IsSnowman())
		{
			if (info->My_State == ST_SNOWMAN)
			{
				info->current_snow_count = 0;
				Reset_Items(player_->iSessionId);
				player_->ChangeSnowman();
			}
		}
		else if (player_->IsSnowman())
		{
			if (info->My_State == ST_ANIMAL)
			{
				Reset_Items(player_->iSessionId);
				player_->ChangeAnimal();
			}
		}
		// 캐릭터 속성 업데이트
		if (player_->iCurrentSnowballCount != info->current_snow_count)
		{
			//MYLOG(Warning, TEXT("Player damaged hp: %d"), info.HealthValue);
			player_->iCurrentSnowballCount = info->current_snow_count;
		}
		//if (info->start_farming_item == ITEM_MAT);
		//if (info->start_farming_item == ITEM_UMB);
		//if (info->start_farming_item == ITEM_BAG);
		//if (info->start_farming_item == ITEM_SNOW)
		//{
		//	player_->StartFarming();
		//	info->start_farming_item = -1;
		//}
		//if (info->end_farming == true)
		//{
		//	player_->EndFarming();
		//	info->end_farming = false;
		//}
	}
	return true;
}

void AMyPlayerController::UpdateNewPlayer()
{
	UWorld* const World = GetWorld();

	int size_ = newPlayers.Size();


	// 새로운 플레이어를 필드에 스폰
	FVector SpawnLocation_;
	SpawnLocation_.X = newplayer.get()->X;
	SpawnLocation_.Y = newplayer.get()->Y;
	SpawnLocation_.Z = newplayer.get()->Z;

	FRotator SpawnRotation;
	SpawnRotation.Yaw = newplayer.get()->Yaw;
	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.Name = FName(*FString(to_string(newplayer.get()->SessionId).c_str()));

	WhoToSpawn = AMyCharacter::StaticClass();
	AMyCharacter* SpawnCharacter = World->SpawnActor<AMyCharacter>(WhoToSpawn, SpawnLocation_, SpawnRotation, SpawnParams);
	SpawnCharacter->SpawnDefaultController();
	SpawnCharacter->iSessionId = newplayer.get()->SessionId;
	SpawnCharacter->SetCharacterMaterial(SpawnCharacter->iSessionId);

	// 필드의 플레이어 정보에 추가
	if (charactersInfo != nullptr)
	{
		cCharacter info;
		info.SessionId = newplayer.get()->SessionId;
		info.X = newplayer.get()->X;
		info.Y = newplayer.get()->Y;
		info.Z = newplayer.get()->Z;

		info.Yaw = newplayer.get()->Yaw;

		charactersInfo->players[newplayer.get()->SessionId] = info;
	}

	newplayer = NULL;

	//MYLOG(Warning, TEXT("other player(id : %d) spawned."), newPlayers.front()->SessionId);

	//bNewPlayerEntered = false;
}


void AMyPlayerController::Reset_Items(int s_id)
{
	// 필드의 플레이어 정보에 추가
	if (charactersInfo != nullptr)
	{
		charactersInfo->players[s_id].current_snow_count = 0;
		//charactersInfo->players[s_id].current_match_count = 0;
		//charactersInfo->players[s_id].bHasUmbrella = false;
		//charactersInfo->players[s_id].bHasBag = false;;
	
	}
	/*iCurrentSnowballCount = 0;
	iMaxSnowballCount = iOriginMaxSnowballCount;
	iCurrentMatchCount = 0;
	iMaxMatchCount = iOriginMaxMatchCount;
	bHasUmbrella = false;
	bHasBag = false;*/

	//UpdateUI(UICategory::AllOfUI);
}

void AMyPlayerController::SendPlayerInfo(int input)
{
	if (!localPlayerCharacter)
		return;
	auto loc = localPlayerCharacter->GetActorLocation();
	float fNewYaw = localPlayerCharacter->GetActorRotation().Yaw;		//yaw 값만 필요함
	auto vel = localPlayerCharacter->GetVelocity();
	FVector MyCameraLocation;
	FRotator MyCameraRotation;
	localPlayerCharacter->GetActorEyesViewPoint(MyCameraLocation, MyCameraRotation);
	float dir = localPlayerCharacter->GetAnim()->GetDirection();

	if (input == COMMAND_MOVE) 
	mySocket->Send_MovePacket(iSessionId, loc, fNewYaw, vel, dir);
	else if (input == COMMAND_ATTACK)
		mySocket->Send_Throw_Packet(iSessionId, MyCameraLocation, MyCameraRotation.Vector());
	else if (input == COMMAND_DAMAGE)
		mySocket->Send_DamagePacket();
	else if (input == COMMAND_MATCH)
		mySocket->Send_MatchPacket();
}

//플레이어 정보 업데이트
void AMyPlayerController::UpdatePlayerInfo(cCharacter& info)
{
	UWorld* const world = GetWorld();
	if (!world)
		return;

	auto player_ = Cast<AMyCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!player_)
		return;

	if (!info.IsAlive)
	{
		/*UE_LOG(LogClass, Log, TEXT("Player Die"));
		FTransform transform(Player->GetActorLocation());
		UGameplayStatics::SpawnEmitterAtLocation(
			world, DestroyEmiiter, transform, true
		);
		Player->Destroy();

		CurrentWidget->RemoveFromParent();
		GameOverWidget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
		if (GameOverWidget != nullptr)
		{
			GameOverWidget->AddToViewport();
		}*/
	}
	else
	{
		if (bisBone) { 
			player_->UpdateTemperatureState();
			bisBone = false;
		}
		//눈사람 변화
		if (player_->IsSnowman())
		{
			if (info.My_State == ST_ANIMAL)
			{
				Reset_Items(player_->iSessionId);
				info.HealthValue = player_->iBeginSlowHP;
				player_->ChangeAnimal();
			}
		}
		else if (!player_->IsSnowman())
		{
			if (info.My_State == ST_SNOWMAN)
			{
				Reset_Items(player_->iSessionId);
				info.HealthValue = player_->iMinHP;
				player_->ChangeSnowman();
			}
		}
		// 캐릭터 속성 업데이트
		if (player_->iCurrentHP != info.HealthValue)
		{
			//MYLOG(Warning, TEXT("Player damaged hp: %d"), info.HealthValue);
			player_->iCurrentHP = info.HealthValue;
			CallDelegateUpdateHP();

		}
		// 캐릭터 속성 업데이트
		if (player_->iCurrentSnowballCount != info.current_snow_count)
		{
			//MYLOG(Warning, TEXT("Player damaged hp: %d"), info.HealthValue);
			player_->iCurrentSnowballCount = info.current_snow_count;
			CallDelegateUpdateCurrentSnowballCount();
		}

	}
}

//void AMyPlayerController::SendFarming(int item_no)
//{
//		mySocket->Send_ItemPacket(item_no);
//}

void AMyPlayerController::LoadReadyUI()
{
	if (readyUIClass)
	{
		readyUI = CreateWidget<UUserWidget>(GetWorld(), readyUIClass);
		if (readyUI)
		{
			readyUI->AddToViewport();

			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
	}
}

void AMyPlayerController::PlayerReady()
{
	// 서버에 레디했다고 전송
	mySocket->Send_ReadyPacket();
	UE_LOG(LogTemp, Warning, TEXT("PlayerReady"));
	bIsReady = true;
#ifdef SINGLEPLAY_DEBUG
	StartGame();	// 디버깅용 - 레디버튼 누르면 startgame 호출
#endif
}

void AMyPlayerController::PlayerUnready()
{
	// 서버에 언레디했다고 전송
	UE_LOG(LogTemp, Warning, TEXT("PlayerUnready"));
	bIsReady = false;
}

void AMyPlayerController::StartGame()
{
	if (!bInGame) {
		UE_LOG(LogTemp, Warning, TEXT("StartGame"));
		bInGame = true;
		readyUI->RemoveFromParent();	// ReadyUI 제거
		LoadCharacterUI(); // CharacterUI 띄우기
		// 실행시 클릭없이 바로 조작
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		auto m_Player = Cast<AMyCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		m_Player->UpdateTemperatureState();
	}
	// 게임 시작되면 실행시킬 코드들 작성
}

void AMyPlayerController::LoadCharacterUI()
{
	if (characterUIClass)
	{
		characterUI = CreateWidget<UUserWidget>(GetWorld(), characterUIClass);
		if (characterUI)
		{
			characterUI->AddToViewport();
			CallDelegateUpdateAllOfUI();	// 모든 캐릭터 ui 갱신
		}
	}
}

// 게임 종료 시 결과창 ui 띄우기 (승자 id 인자로 받아서 승자, 패자 ui 다르게 뜨도록)
void AMyPlayerController::LoadGameResultUI(int winnerSessionId)
{	// remove character ui 
	if (gameResultUIClass)
	{
		gameResultUI = CreateWidget<UUserWidget>(GetWorld(), gameResultUIClass);
		if (gameResultUI)
		{
			characterUI->RemoveFromParent();	// character ui 제거
			gameResultUI->AddToViewport();		// game result ui 띄우기
			
			bool bIsWinner = (iSessionId == winnerSessionId) ? true : false;
			CallDelegateUpdateGameResult(bIsWinner);

			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
	}
}

void AMyPlayerController::CallDelegateUpdateAllOfUI()
{
	CallDelegateUpdateHP();
	CallDelegateUpdateCurrentSnowballCount();
	CallDelegateUpdateCurrentMatchCount();
	CallDelegateUpdateMaxSnowballAndMatchCount();
	CallDelegateUpdateHasUmbrella();
	CallDelegateUpdateHasBag();

	CallDelegateUpdateSelectedItem();
}

void AMyPlayerController::CallDelegateUpdateHP()
{
	if (!characterUI) return;	// CharacterUI가 생성되기 전이면 갱신 x

	if (FuncUpdateHP.IsBound() == true) FuncUpdateHP.Broadcast(localPlayerCharacter->iCurrentHP);	// 델리게이트 호출

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update hp %d"), localPlayer->iCurrentHP);
}

void AMyPlayerController::CallDelegateUpdateCurrentSnowballCount()
{
	if (!characterUI) return;

	if (FuncUpdateCurrentSnowballCount.IsBound() == true) FuncUpdateCurrentSnowballCount.Broadcast(localPlayerCharacter->iCurrentSnowballCount);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update current snowball count %d"), localPlayerCharacter->iCurrentSnowballCount);
}
void AMyPlayerController::CallDelegateUpdateCurrentMatchCount()
{
	if (!characterUI) return;

	if (FuncUpdateCurrentMatchCount.IsBound() == true) FuncUpdateCurrentMatchCount.Broadcast(localPlayerCharacter->iCurrentMatchCount);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update current match count %d"), localPlayerCharacter->iCurrentMatchCount);
}
void AMyPlayerController::CallDelegateUpdateMaxSnowballAndMatchCount()
{
	if (!characterUI) return;

	if (FuncUpdateMaxSnowballAndMatchCount.IsBound() == true)
		FuncUpdateMaxSnowballAndMatchCount.Broadcast(localPlayerCharacter->iMaxSnowballCount, localPlayerCharacter->iMaxMatchCount);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update max snowball count %d, max match count %d"), 
	//	localPlayerCharacter->iMaxSnowballCount, localPlayerCharacter->iMaxMatchCount);
}
void AMyPlayerController::CallDelegateUpdateHasUmbrella()
{
	if (!characterUI) return;

	if (FuncUpdateHasUmbrella.IsBound() == true) FuncUpdateHasUmbrella.Broadcast(localPlayerCharacter->bHasUmbrella);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update has umbrella %d"), localPlayerCharacter->bHasUmbrella);
}
void AMyPlayerController::CallDelegateUpdateHasBag()
{
	if (!characterUI) return;

	if (FuncUpdateHasBag.IsBound() == true) FuncUpdateHasBag.Broadcast(localPlayerCharacter->bHasBag);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update has bag %d"), localPlayerCharacter->bHasBag);
}

void AMyPlayerController::CallDelegateUpdateIsFarmingSnowdrift()
{
	if (!characterUI) return;

	if (FuncUpdateIsFarmingSnowdrift.IsBound() == true) FuncUpdateIsFarmingSnowdrift.Broadcast(localPlayerCharacter->GetIsFarming());

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update is farming snowdrift %d"), localPlayerCharacter->GetIsFarming());
}

void AMyPlayerController::CallDelegateUpdateSnowdriftFarmDuration(float farmDuration)
{
	if (!characterUI) return;

	if (FuncUpdateSnowdriftFarmDuration.IsBound() == true) FuncUpdateSnowdriftFarmDuration.Broadcast(farmDuration);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update snowdrift farm duration %f"), farmDuration);
}

void AMyPlayerController::CallDelegateUpdateSelectedItem()
{
	if (!characterUI) return;

	if (FuncUpdateSelectedItem.IsBound() == true) FuncUpdateSelectedItem.Broadcast(localPlayerCharacter->iSelectedItem);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update selected item %d"), localPlayerCharacter->iSelectedItem);
}

void AMyPlayerController::CallDelegateUpdateGameResult(bool isWinner)
{
	if (!gameResultUI) return;
	
	if (FuncUpdateGameResult.IsBound() == true) FuncUpdateGameResult.Broadcast(isWinner);

	//UE_LOG(LogTemp, Warning, TEXT("call delegate update selected item %d"), localPlayerCharacter->iSelectedItem);
}
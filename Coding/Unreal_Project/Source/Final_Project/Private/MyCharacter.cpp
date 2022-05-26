// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "MyAnimInstance.h"
#include "MyPlayerController.h"
#include "Snowdrift.h"
#include "Debug.h"


const int AMyCharacter::iMaxHP = 390;
const int AMyCharacter::iMinHP = 270;
const int AMyCharacter::iBeginSlowHP = 300;	// 캐릭터가 슬로우 상태가 되기 시작하는 hp
const int iNormalSpeed = 600;	// 캐릭터 기본 이동속도
const int iSlowSpeed = 400;		// 캐릭터 슬로우 상태 이동속도
const float fChangeSnowmanStunTime = 3.0f;	// 실제값 - 10.0f, 눈사람화 할 때 스턴 시간
const float fStunTime = 3.0f;	// 눈사람이 눈덩이 맞았을 때 스턴 시간
const int iOriginMaxSnowballCount = 10;	// 눈덩이 최대보유량 (초기, 가방x)
const int iOriginMaxMatchCount = 2;	// 성냥 최대보유량 (초기, 가방x)
const int iNumOfWeapons = 2;	// 무기 종류 수

// 색상별 곰 텍스쳐
FString TextureStringArray[] = {
	TEXT("/Game/Characters/Bear/bear_texture.bear_texture"),
	TEXT("/Game/Characters/Bear/bear_texture_light_red.bear_texture_light_red"),
	TEXT("/Game/Characters/Bear/bear_texture_yellow.bear_texture_yellow"),
	TEXT("/Game/Characters/Bear/bear_texture_light_green.bear_texture_light_green"),
	TEXT("/Game/Characters/Bear/bear_texture_cyan.bear_texture_cyan"),
	TEXT("/Game/Characters/Bear/bear_texture_blue.bear_texture_blue"),
	TEXT("/Game/Characters/Bear/bear_texture_light_gray.bear_texture_light_gray"),
	TEXT("/Game/Characters/Bear/bear_texture_black.bear_texture_black") };




// Sets default values
AMyCharacter::AMyCharacter()
{
	
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	springArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
	camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));
	check(camera != nullptr);

	springArm->SetupAttachment(CastChecked<USceneComponent, UCapsuleComponent>(GetCapsuleComponent()));
	camera->SetupAttachment(springArm);

	GetCapsuleComponent()->SetCapsuleHalfHeight(74.0f);
	GetCapsuleComponent()->SetCapsuleRadius(37.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("MyCharacter"));
	GetCapsuleComponent()->SetUseCCD(true);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AMyCharacter::OnHit);
	GetCapsuleComponent()->BodyInstance.bNotifyRigidBodyCollision = true;
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -77.0f), FRotator(0.0f, -90.0f, 0.0f));
	springArm->TargetArmLength = 220.0f;
	springArm->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 50.0f), FRotator::ZeroRotator);
	springArm->bUsePawnControlRotation = true;
	springArm->bInheritPitch = true;
	springArm->bInheritRoll = true;
	springArm->bInheritYaw = true;
	springArm->bDoCollisionTest = true;
	springArm->SocketOffset.Y = 30.0f;
	springArm->SocketOffset.Z = 35.0f;
	bUseControllerRotationYaw = true;

	bear = CreateDefaultSubobject<USkeletalMesh>(TEXT("BEAR"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_BEAR(TEXT("/Game/Characters/Bear/bear.bear"));
	if (SK_BEAR.Succeeded())
	{
		bear = SK_BEAR.Object;
		GetMesh()->SetSkeletalMesh(bear);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	snowman = CreateDefaultSubobject<USkeletalMesh>(TEXT("SNOWMAN"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_SNOWMAN(TEXT("/Game/Characters/Snowman/snowman.snowman"));
	if (SK_SNOWMAN.Succeeded())
	{
		snowman = SK_SNOWMAN.Object;
	}
	
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	static ConstructorHelpers::FClassFinder<UAnimInstance> BEAR_ANIM(TEXT("/Game/Animations/Bear/BearAnimBP.BearAnimBP_C"));	// _C 붙이기
	if (BEAR_ANIM.Succeeded())
	{
		bearAnim = BEAR_ANIM.Class;
		GetMesh()->SetAnimInstanceClass(BEAR_ANIM.Class);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> SNOWMAN_ANIM(TEXT("/Game/Animations/Snowman/SnowmanAnimBP.SnowmanAnimBP_C"));
	if (SNOWMAN_ANIM.Succeeded())
	{
		snowmanAnim = SNOWMAN_ANIM.Class;
	}

	static ConstructorHelpers::FObjectFinder<UMaterial>BearMaterial(TEXT("/Game/Characters/Bear/M_Bear.M_Bear"));
	if (BearMaterial.Succeeded())
	{
		bearMaterial = BearMaterial.Object;
	}

	// 모든 색상의 곰 텍스쳐 로드해서 저장
	for (int i = 0; i < 8; ++i)
	{
		ConstructorHelpers::FObjectFinder<UTexture>BearTexture(*(TextureStringArray[i]));
		if (BearTexture.Succeeded())
		{
			bearTextureArray.Add(BearTexture.Object);
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterial>SnowmanMaterial(TEXT("/Game/Characters/Snowman/M_Snowman.M_Snowman"));
	if (SnowmanMaterial.Succeeded())
	{
		snowmanMaterial = SnowmanMaterial.Object;
	}

	if (!shotgunMeshComponent)
	{
		shotgunMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("shotgunMeshComponent"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_SHOTGUN(TEXT("/Game/NonCharacters/Shotgun_SM.Shotgun_SM"));
		if (SM_SHOTGUN.Succeeded())
		{
			shotgunMeshComponent->SetStaticMesh(SM_SHOTGUN.Object);
			shotgunMeshComponent->BodyInstance.SetCollisionProfileName(TEXT("NoCollision"));

			FAttachmentTransformRules atr = FAttachmentTransformRules(
				EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, EAttachmentRule::KeepWorld, true);
			shotgunMeshComponent->AttachToComponent(GetMesh(), atr, TEXT("ShotgunSocket"));
			
			shotgunMeshComponent->SetVisibility(false);
		}
	}

	GetCharacterMovement()->JumpZVelocity = 800.0f;
	isAttacking = false;

	projectileClass = AMySnowball::StaticClass();
	shotgunProjectileClass = AMySnowball::StaticClass();	// snowball bomb 클래스 제작해서 변경해야함

	iSessionId = -1;
	iCurrentHP = iMaxHP;	// 실제 설정값
	//iCurrentHP = iMinHP + 1;	// 디버깅용 - 대기시간 후 눈사람으로 변화

	snowball = nullptr;
	
	iMaxSnowballCount = iOriginMaxSnowballCount;
	iCurrentSnowballCount = 0;	// 실제 설정값
	//iCurrentSnowballCount = 10;	// 디버깅용
	iMaxMatchCount = iOriginMaxMatchCount;
	iCurrentMatchCount = 0;
	bHasUmbrella = false;
	bHasBag = false;

	farmingItem = nullptr;
	bIsFarming = false;
	
	bIsInsideOfBonfire = false;

	iCharacterState = CharacterState::AnimalNormal;
	bIsSnowman = false;
	GetCharacterMovement()->MaxWalkSpeed = iNormalSpeed;	// 캐릭터 이동속도 설정
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->PushForceFactor = 75000.0f;		// 기본값 750000.0f
	GetCharacterMovement()->MaxAcceleration = 1024.0f;		// 기본값 2048.0f

	iSelectedItem = ItemTypeList::Match;	// 선택된 아이템 기본값 - 성냥
	
	bIsInTornado = false;
	rotateCont = false;

	iSelectedWeapon = Weapon::Hand;	// 실제 설정값
	//iSelectedWeapon = Weapon::Shotgun;	// 디버깅용 - 샷건
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 수정 필요 - 캐릭터의 session id가 결정될 때 이 함수가 호출되도록
	SetCharacterMaterial(iSessionId);	// 캐릭터 머티리얼 설정(색상)

	playerController = Cast<APlayerController>(GetController());	// 생성자에서 하면 x (컨트롤러가 생성되기 전인듯)
	localPlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	
	WaitForStartGame();	// 대기시간
}

void AMyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MYLOG(Warning, TEXT("endplay"));
	//AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	//PlayerController->GetSocket()->StopListen();

	if (iSessionId == localPlayerController->iSessionId)
	{
		localPlayerController->mySocket->Send_LogoutPacket(iSessionId);
		SleepEx(0, true);
		localPlayerController->mySocket->CloseSocket();
		localPlayerController->mySocket->~ClientSocket();
		//delete[] localPlayerController->mySocket;
	}
 }


// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bIsSnowman)
	{
		UpdateFarming(DeltaTime);
		UpdateHP();
		UpdateSpeed();
	}

	UpdateZByTornado();		// 캐릭터가 토네이도 내부인 경우 z값 증가
	UpdateControllerRotateByTornado();	// 토네이도로 인한 스턴상태인 경우 회전하도록
}

void AMyCharacter::init_Socket()
{
	/*AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayerController->mySocket = nullptr;
	PlayerController->mySocket = new ClientSocket();
	PlayerController->mySocket->SetPlayerController(PlayerController);
	g_socket = PlayerController->mySocket;
	PlayerController->mySocket->Connect();
	
	DWORD recv_flag = 0;
	ZeroMemory(&g_socket->_recv_over._wsa_over, sizeof(g_socket->_recv_over._wsa_over));
	g_socket->_recv_over._wsa_buf.buf = reinterpret_cast<char*>(g_socket->_recv_over._net_buf + g_socket->_prev_size);
	g_socket->_recv_over._wsa_buf.len = sizeof(g_socket->_recv_over._net_buf) - g_socket->_prev_size;
	WSARecv(g_socket->_socket, &g_socket->_recv_over._wsa_buf, 1, 0, &recv_flag, &g_socket->_recv_over._wsa_over, recv_callback);
	g_socket->Send_LoginPacket();
	SleepEx(0, true);*/
}

void AMyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	myAnim = Cast<UMyAnimInstance>(GetMesh()->GetAnimInstance());
	MYCHECK(nullptr != myAnim);

	myAnim->OnMontageEnded.AddDynamic(this, &AMyCharacter::OnAttackMontageEnded);
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("UpDown"), this, &AMyCharacter::UpDown);
	PlayerInputComponent->BindAxis(TEXT("LeftRight"), this, &AMyCharacter::LeftRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AMyCharacter::LookUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AMyCharacter::Turn);

	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Attack"), EInputEvent::IE_Pressed, this, &AMyCharacter::Attack);
	PlayerInputComponent->BindAction(TEXT("Farming"), EInputEvent::IE_Pressed, this, &AMyCharacter::StartFarming);
	PlayerInputComponent->BindAction(TEXT("Farming"), EInputEvent::IE_Released, this, &AMyCharacter::EndFarming);

	PlayerInputComponent->BindAction(TEXT("SelectMatch"), EInputEvent::IE_Pressed, this, &AMyCharacter::SelectMatch);
	PlayerInputComponent->BindAction(TEXT("SelectUmbrella"), EInputEvent::IE_Pressed, this, &AMyCharacter::SelectUmbrella);
	PlayerInputComponent->BindAction(TEXT("UseSelectedItem"), EInputEvent::IE_Pressed, this, &AMyCharacter::UseSelectedItem);

	PlayerInputComponent->BindAction(TEXT("ChangeWeapon"), EInputEvent::IE_Pressed, this, &AMyCharacter::ChangeWeapon);
}

void AMyCharacter::UpDown(float NewAxisValue)
{
	AddMovementInput(GetActorForwardVector(), NewAxisValue);
}

void AMyCharacter::LeftRight(float NewAxisValue)
{
	AddMovementInput(GetActorRightVector(), NewAxisValue);
}

void AMyCharacter::LookUp(float NewAxisValue)
{
	AddControllerPitchInput(NewAxisValue);
}

void AMyCharacter::Turn(float NewAxisValue)
{
	AddControllerYawInput(NewAxisValue);
}

void AMyCharacter::Attack()
{
	if (isAttacking) return;
	MYLOG(Warning, TEXT("attack"));
	if (bIsSnowman) return;
	if (iCurrentSnowballCount <= 0) return;	// 눈덩이를 소유하고 있지 않으면 공격 x
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (iSessionId == PlayerController->iSessionId)
	{
		switch (iSelectedWeapon) {
		case Weapon::Hand:
			PlayerController->SendPlayerInfo(COMMAND_ATTACK);
			isAttacking = true;
			break;
		case Weapon::Shotgun:
			if (iCurrentSnowballCount < 4) return;	// 눈덩이가 4개 이상 없으면 공격 x
			AttackShotgun();
			isAttacking = true;
			break;
		default:
			break;
		}
	}
#ifdef SINGLEPLAY_DEBUG
	SnowAttack();
#endif
}

void AMyCharacter::SnowAttack()
{
	//if (bIsSnowman) return;
	//if (iCurrentSnowballCount <= 0) return;	// 눈덩이를 소유하고 있지 않으면 공격 x

	myAnim->PlayAttackMontage();

	// 디버깅용 - 실제는 주석 해제
	//iCurrentSnowballCount -= 1;	// 공격 시 눈덩이 소유량 1 감소
	UpdateUI(UICategory::CurSnowball);

	// Attempt to fire a projectile.
	if (projectileClass)
	{
		UWorld* World = GetWorld();

		if (World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();

			FTransform SnowballSocketTransform = GetMesh()->GetSocketTransform(TEXT("SnowballSocket"));
			snowball = World->SpawnActor<AMySnowball>(projectileClass, SnowballSocketTransform, SpawnParams);
			FAttachmentTransformRules atr = FAttachmentTransformRules(
				EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, EAttachmentRule::KeepWorld, true);
			snowball->AttachToComponent(GetMesh(), atr, TEXT("SnowballSocket"));
		}
	}
}

void AMyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	isAttacking = false;
}

float AMyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (!bIsSnowman)
	{	// 동물인 경우 체력 감소
#ifdef SINGLEPLAY_DEBUG
		iCurrentHP = FMath::Clamp<int>(iCurrentHP - FinalDamage, iMinHP, iMaxHP);
		UpdateUI(UICategory::HP);	// 변경된 체력으로 ui 갱신

		MYLOG(Warning, TEXT("Actor : %s took Damage : %f, HP : %d"), *GetName(), FinalDamage, iCurrentHP);
#endif
		if (iSessionId == localPlayerController->iSessionId)
		{
			localPlayerController->SendPlayerInfo(COMMAND_DAMAGE);
		}
	}
	else
	{	// 눈사람인 경우 스턴
		StartStun(fStunTime);
#ifdef SINGLEPLAY_DEBUG
		MYLOG(Warning, TEXT("Actor : %s stunned, HP : %d"), *GetName(), iCurrentHP);
#endif
	}
	return FinalDamage;
}

void AMyCharacter::SendReleaseSnowball()
{
	if (iSessionId != localPlayerController->iSessionId) return;
	if (IsValid(snowball))
	{
		localPlayerController->SendPlayerInfo(COMMAND_THROW);

	}
}

void AMyCharacter::ReleaseSnowball()
{
	if (IsValid(snowball))
	{
		FDetachmentTransformRules dtr = FDetachmentTransformRules(EDetachmentRule::KeepWorld, false);
		snowball->DetachFromActor(dtr);

		if (snowball->GetClass()->ImplementsInterface(UI_Throwable::StaticClass()))
		{
			//던지는 순간 좌표값 보내는 코드
#ifdef MULTIPLAY_DEBUG
			AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
			
			FVector direction_;
			direction_.X = PlayerController->GetCharactersInfo()->players[iSessionId].fCDx;
			direction_.Y = PlayerController->GetCharactersInfo()->players[iSessionId].fCDy;
			direction_.Z = PlayerController->GetCharactersInfo()->players[iSessionId].fCDz;

			II_Throwable::Execute_Throw(snowball, direction_);
#endif
#ifdef SINGLEPLAY_DEBUG
			FVector cameraLocation;
			FRotator cameraRotation;
			GetActorEyesViewPoint(cameraLocation, cameraRotation);
			AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());

			II_Throwable::Execute_Throw(snowball, cameraRotation.Vector());
#endif
			snowball = nullptr;
		}

	}
}

void AMyCharacter::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
#ifdef MULTIPLAY_DEBUG
	if (!iSessionId == PlayerController->iSessionId || !PlayerController->is_start()) return;
#endif

	auto MySnowball = Cast<AMySnowball>(OtherActor);

	if (nullptr != MySnowball)
	{
		//AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
		//if (iSessionId == PlayerController->iSessionId)
		//{
		//	MYLOG(Warning, TEXT("id: %d snowball hit."), iSessionId);

		//	PlayerController->UpdatePlayerInfo(COMMAND_DAMAGE);
		//}

	}
	AMyCharacter* otherCharacter = Cast<AMyCharacter>(OtherActor);
	if (!otherCharacter) return;

	// 자신 - 눈사람, 스턴상태 x
	// 상대 - 동물
	if (bIsSnowman && iCharacterState != CharacterState::SnowmanStunned)
	{
		if (!(otherCharacter->GetIsSnowman()))
		{	// 본인 동물화(부활), 상대 캐릭터 눈사람화(사망)
#ifdef MULTIPLAY_DEBUG
			//bIsSnowman = false;
			//PlayerController->SetCharacterState(iSessionId, ST_ANIMAL);
			//PlayerController->SetCharacterHP(iSessionId, iMaxHP);
			//ChangeAnimal();
			PlayerController->GetSocket()->Send_StatusPacket(ST_ANIMAL, iSessionId);
			//UpdateTemperatureState();
			PlayerController->GetSocket()->Send_StatusPacket(ST_SNOWMAN, otherCharacter->iSessionId);
			
			//PlayerController->SetCharacterState(iSessionId, ST_ANIMAL);
			//otherCharacter->ChangeSnowman();
			//PlayerController->SetCharacterState(otherCharacter->iSessionId, ST_SNOWMAN);
#endif
#ifdef SINGLEPLAY_DEBUG
			ChangeAnimal();
			otherCharacter->ChangeSnowman();
#endif
			UE_LOG(LogTemp, Warning, TEXT("%s catch %s"), *GetName(), *(otherCharacter->GetName()));
			return;
		}
	}
	/*else if (!bIsSnowman && otherCharacter->GetIsSnowman())
	{
		bIsSnowman = true;
		PlayerController->SetCharacterState(iSessionId, ST_SNOWMAN);
		ChangeSnowman();
		if (iSessionId == PlayerController->iSessionId && PlayerController->is_start())
			PlayerController->GetSocket()->Send_StatusPacket(ST_SNOWMAN);
		
	}*/

	//auto MyCharacter = Cast<AMyCharacter>(OtherActor);

	//if (nullptr != MyCharacter)
	//{
	//	FDamageEvent DamageEvent;
	//	MyCharacter->TakeDamage(iDamage, DamageEvent, false, this);
	//}
}

void AMyCharacter::StartFarming()
{
	if (!IsValid(farmingItem)) return;	//오버랩하면 바로 넣어줌
	if (bIsSnowman) return;
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (iSessionId != PlayerController->iSessionId || !PlayerController->is_start()) return;
	if (Cast<ASnowdrift>(farmingItem))
	{
		if (iCurrentSnowballCount >= iMaxSnowballCount) return;	// 눈덩이 최대보유량 이상은 눈 무더기 파밍 못하도록
		bIsFarming = true;
		UpdateUI(UICategory::IsFarmingSnowdrift);
	}
	else if (Cast<AItembox>(farmingItem))
	{
		AItembox* itembox = Cast<AItembox>(farmingItem);
		switch (itembox->GetItemboxState())
		{
		case ItemboxState::Closed:
			itembox->SetItemboxState(ItemboxState::Opening);
			PlayerController->GetSocket()->Send_OpenBoxPacket(itembox->GetId());
			break;
		case ItemboxState::Opened:
			// 아이템박스에서 내용물 파밍에 성공하면 아이템박스에서 아이템 제거 (박스는 그대로 유지시킴)
			if (GetItem(itembox->GetItemType())) { 
				MYLOG(Warning, TEXT("item %d"), itembox->GetItemType());


				//아이템 파밍 시 서버 전송
#ifdef MULTIPLAY_DEBUG
				PlayerController->GetSocket()->Send_ItemPacket(itembox->GetItemType(), itembox->GetId());
#endif

				itembox->DeleteItem(); //서버에서 패킷받았을 때 처리
				//박스가 열리는 시점에서 아이템 동기화
			}
			break;
		// 아이템박스가 열리는 중이거나 비어있는 경우
		case ItemboxState::Opening:
			break;
		case ItemboxState::Empty:
			break;
		default:
			break;
		}
	}
}

bool AMyCharacter::GetItem(int itemType)
{
	
	switch (itemType) {
	case ItemTypeList::Match:
		if (iCurrentMatchCount >= iMaxMatchCount) return false;	// 성냥 최대보유량을 넘어서 파밍하지 못하도록
		iCurrentMatchCount += 1;
		UpdateUI(UICategory::CurMatch);
		return true;
		break;
	case ItemTypeList::Umbrella:
		if (bHasUmbrella) return false;	// 우산을 소유 중이면 우산 파밍 못하도록
		bHasUmbrella = true;
		UpdateUI(UICategory::HasUmbrella);
		return true;
		break;
	case ItemTypeList::Bag:
		if (bHasBag) return false;	// 가방을 소유 중이면 가방 파밍 못하도록
		bHasBag = true;
		iMaxSnowballCount += 5;	// 눈덩이 10 -> 15 까지 보유 가능
		iMaxMatchCount += 1;	// 성냥 2 -> 3 까지 보유 가능
		UpdateUI(UICategory::HasBag);
		UpdateUI(UICategory::MaxSnowballAndMatch);
		return true;
		break;
	default:
		return false;
		break;
	}
}

void AMyCharacter::EndFarming()
{
	if (bIsSnowman) return;

	if (IsValid(farmingItem))
	{
		if (Cast<ASnowdrift>(farmingItem))
		{	

			if (iCurrentSnowballCount >= iMaxSnowballCount) return;
			
			// F키로 눈 무더기 파밍 중 F키 release 시 눈 무더기 duration 초기화
			ASnowdrift* snowdrift = Cast<ASnowdrift>(farmingItem);
			snowdrift->SetFarmDuration(ASnowdrift::fFarmDurationMax);
			bIsFarming = false;
			UpdateUI(UICategory::IsFarmingSnowdrift);
		}
	}
}

void AMyCharacter::UpdateFarming(float deltaTime)
{
	if (!bIsFarming) return;
	if (!IsValid(farmingItem)) return;
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (iSessionId != PlayerController->iSessionId || !PlayerController->is_start()) return;
	ASnowdrift* snowdrift = Cast<ASnowdrift>(farmingItem);
	if (snowdrift)
	{	// 눈 무더기 duration 만큼 F키를 누르고 있으면 캐릭터의 눈덩이 추가 
		float lastFarmDuration = snowdrift->GetFarmDuration();
		float newFarmDuration = lastFarmDuration - deltaTime;
		snowdrift->SetFarmDuration(newFarmDuration);
		UpdateUI(UICategory::SnowdriftFarmDuration, newFarmDuration);

		if (newFarmDuration <= 0)
		{
			//눈덩이 파밍 시 서버 전송
#ifdef MULTIPLAY_DEBUG
			PlayerController->GetSocket()->Send_ItemPacket(ITEM_SNOW, snowdrift->GetId());
#endif
			UpdateUI(UICategory::CurSnowball);
			bIsFarming = false;	// 눈무더기 파밍 끝나면 false로 변경 후 UI 갱신 (눈무더기 파밍 바 ui 안보이도록)
			UpdateUI(UICategory::IsFarmingSnowdrift);
			//snowdrift->Destroy(); //서버에서 아이디 반환시 처리
			//snowdrift = nullptr;
		}
	}
}

void AMyCharacter::UpdateHP()
{
	if (iCurrentHP <= iMinHP)
	{
		ChangeSnowman();
	}
}

void AMyCharacter::UpdateSpeed()
{
	switch (iCharacterState) {
	case CharacterState::AnimalNormal:
		if (iCurrentHP <= iBeginSlowHP)
		{
			iCharacterState = CharacterState::AnimalSlow;
			GetCharacterMovement()->MaxWalkSpeed = iSlowSpeed;
		}
		break;
	case CharacterState::AnimalSlow:
		if (iCurrentHP > iBeginSlowHP)
		{
			iCharacterState = CharacterState::AnimalNormal;
			GetCharacterMovement()->MaxWalkSpeed = iNormalSpeed;
		}
		break;
	default:
		break;
	}
}

void AMyCharacter::ChangeSnowman()
{
	bIsSnowman = true;

	// 스켈레탈메시, 애니메이션 블루프린트 변경
	myAnim->SetDead();
	GetMesh()->SetSkeletalMesh(snowman);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetAnimInstanceClass(snowmanAnim);
	SetCharacterMaterial();	// 눈사람으로 머티리얼 변경할 때는 색상 필요 x (디폴트값으로)

	iCurrentHP = iMinHP;
	GetWorldTimerManager().ClearTimer(temperatureHandle);	// 기존에 실행중이던 체온 증감 핸들러 초기화 (체온 변화하지 않도록)
	ResetHasItems();
	UpdateUI(UICategory::AllOfUI);
	

#ifdef SINGLEPLAY_DEBUG
	UpdateTemperatureState();
	UpdateUI(UICategory::HP);	// 변경된 체력으로 ui 갱신
#endif

	GetCharacterMovement()->MaxWalkSpeed = iSlowSpeed;	// 눈사람의 이동속도는 슬로우 상태인 캐릭터와 동일하게 설정
	
	StartStun(fChangeSnowmanStunTime);

	ResetHasItems();

	isAttacking = false;	// 공격 도중에 상태 변할 시 발생하는 오류 방지
}

void AMyCharacter::WaitForStartGame()
{
	//Delay 함수
	FTimerHandle WaitHandle;
	float WaitTime = 3.0f;
	GetWorld()->GetTimerManager().SetTimer(WaitHandle, FTimerDelegate::CreateLambda([&]()
		{
			UpdateTemperatureState();
		}), WaitTime, false);
}

void AMyCharacter::UpdateTemperatureState()
{
	if (bIsSnowman) return;
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (iSessionId != PlayerController->iSessionId || !PlayerController->is_start()) return;
#ifdef SINGLEPLAY_DEBUG
	GetWorldTimerManager().ClearTimer(temperatureHandle);	// 기존에 실행중이던 핸들러 초기화
#endif
	//if (match)
	//{
	//	GetWorldTimerManager().SetTimer(temperatureHandle, this, &AMyCharacter::UpdateTemperatureByMatch, 1.0f, true);
	//}
	//else
	//{
		if (bIsInsideOfBonfire)
		{	// 모닥불 내부인 경우 초당 체온 증가 (초당 호출되는 람다함수)
#ifdef SINGLEPLAY_DEBUG
			GetWorldTimerManager().SetTimer(temperatureHandle, FTimerDelegate::CreateLambda([&]()
				{
					iCurrentHP += ABonfire::iHealAmount;
					iCurrentHP = FMath::Clamp<int>(iCurrentHP, iMinHP, iMaxHP);
					UpdateUI(UICategory::HP);	// 변경된 체력으로 ui 갱신
				}), 1.0f, true);
#endif
			if (iSessionId == PlayerController->iSessionId && PlayerController->is_start())
				PlayerController->GetSocket()->Send_StatusPacket(ST_INBURN, iSessionId);
		}
		else
		{	// 모닥불 외부인 경우 초당 체온 감소 (초당 호출되는 람다함수)
#ifdef SINGLEPLAY_DEBUG
			GetWorldTimerManager().SetTimer(temperatureHandle, FTimerDelegate::CreateLambda([&]()
				{
					iCurrentHP -= ABonfire::iDamageAmount;
					iCurrentHP = FMath::Clamp<int>(iCurrentHP, iMinHP, iMaxHP);
					UpdateUI(UICategory::HP);	// 변경된 체력으로 ui 갱신
				}), 1.0f, true);
#endif
			if (iSessionId == PlayerController->iSessionId && PlayerController->is_start())
				PlayerController->GetSocket()->Send_StatusPacket(ST_OUTBURN, iSessionId);
		}
	//}
}

void AMyCharacter::StartStun(float waitTime)
{
	//if (!playerController) return;	// 다른 플레이어의 캐릭터는 플레이어 컨트롤러가 존재 x?

	if (iCharacterState == CharacterState::SnowmanStunned)
	{	// 스턴 상태에서 또 맞으면 기존에 실행중이던 stunhandle 초기화 (핸들러 새로 갱신하도록)
		GetWorldTimerManager().ClearTimer(stunHandle);
	}

	// 플레이어의 입력을 무시하도록 (움직일 수 없음, 시야도 고정, 상태 - Stunned)
	iCharacterState = bIsSnowman ? CharacterState::SnowmanStunned : CharacterState::AnimalStunned;
	DisableInput(playerController);
	GetMesh()->bPauseAnims = true;	// 캐릭터 애니메이션도 정지

	EndStun(waitTime);	// waitTime이 지나면 stun이 끝나도록

	if (bIsInTornado)
	{	// 토네이도에 의한 스턴이면 캐릭터가 회전하도록, 애니메이션은 재생되도록
		rotateCont = true;
		GetMesh()->bPauseAnims = false;
	}
}

void AMyCharacter::EndStun(float waitTime)
{
	//if (!playerController) return;

	// x초 후에 다시 입력을 받을 수 있도록 (움직임과 시야 제한 해제, 상태 - Snowman)
	GetWorld()->GetTimerManager().SetTimer(stunHandle, FTimerDelegate::CreateLambda([&]()
		{
			iCharacterState = bIsSnowman ? CharacterState::SnowmanNormal : CharacterState::AnimalNormal;
			EnableInput(playerController);
			GetMesh()->bPauseAnims = false;

			rotateCont = false;		// 토네이도에 의해서 회전 중이면 멈추기
		}), waitTime, false);
}

void AMyCharacter::ResetHasItems()
{
	iCurrentSnowballCount = 0;
	iMaxSnowballCount = iOriginMaxSnowballCount;
	iCurrentMatchCount = 0;
	iMaxMatchCount = iOriginMaxMatchCount;
	bHasUmbrella = false;
	bHasBag = false;


	UpdateUI(UICategory::AllOfUI);
}

void AMyCharacter::ChangeAnimal()
{
	bIsSnowman = false;

	// 스켈레탈메시, 애니메이션 블루프린트 변경
	GetMesh()->SetSkeletalMesh(bear);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetAnimInstanceClass(bearAnim);
	SetCharacterMaterial(iSessionId);

	myAnim = Cast<UMyAnimInstance>(GetMesh()->GetAnimInstance());
	MYCHECK(nullptr != myAnim);
	myAnim->OnMontageEnded.AddDynamic(this, &AMyCharacter::OnAttackMontageEnded);
	
	iCurrentHP = iBeginSlowHP;	// 눈사람 -> 동물 부활 시 체력 동상 상태(슬로우)로 설정 (30.0 - 체력의 1/4)
	GetWorldTimerManager().ClearTimer(temperatureHandle);
	ResetHasItems();
	UpdateUI(UICategory::AllOfUI);
	UpdateTemperatureState();
	
#ifdef SINGLEPLAY_DEBUG
	UpdateTemperatureState();
	UpdateUI(UICategory::HP);	// 변경된 체력으로 ui 갱신
#endif

	GetCharacterMovement()->MaxWalkSpeed = iNormalSpeed;

	iCharacterState = CharacterState::AnimalNormal;

	isAttacking = false;	// 공격 도중에 상태 변할 시 발생하는 오류 방지
}

void AMyCharacter::SetCharacterMaterial(int id)
{
	if (id < 0) id = 0;	// id가 유효하지 않은 경우 (싱글플레이)
	if (!bIsSnowman)
	{	// 곰 머티리얼로 변경, 본인 색상의 곰 텍스쳐 적용
		GetMesh()->SetMaterial(0, bearMaterial);
		dynamicMaterialInstance = GetMesh()->CreateDynamicMaterialInstance(0);
		dynamicMaterialInstance->SetTextureParameterValue(FName("Tex"), bearTextureArray[id]);	// 본인 색상의 곰 텍스쳐 사용
	}
	else
	{	// 눈사람 머티리얼로 변경
		GetMesh()->SetMaterial(0, snowmanMaterial);
	}
}

void AMyCharacter::UpdateUI(int uiCategory, float farmDuration)
{
#ifdef MULTIPLAY_DEBUG
	if (iSessionId != localPlayerController->iSessionId) return;	// 로컬플레이어인 경우만 update
#endif
	switch (uiCategory)
	{
	case UICategory::HP:
		localPlayerController->CallDelegateUpdateHP();	// 체력 ui 갱신
		break;
	case UICategory::CurSnowball:
		localPlayerController->CallDelegateUpdateCurrentSnowballCount();	// CurSnowball ui 갱신
		break;
	case UICategory::CurMatch:
		localPlayerController->CallDelegateUpdateCurrentMatchCount();	// CurMatch ui 갱신
		break;
	case UICategory::MaxSnowballAndMatch:
		localPlayerController->CallDelegateUpdateMaxSnowballAndMatchCount();	// MaxSnowballAndMatch ui 갱신
		break;
	case UICategory::HasUmbrella:
		localPlayerController->CallDelegateUpdateHasUmbrella();	// HasUmbrella ui 갱신
		break;
	case UICategory::HasBag:
		localPlayerController->CallDelegateUpdateHasBag();	// HasBag ui 갱신
		break;
	case UICategory::IsFarmingSnowdrift:
		localPlayerController->CallDelegateUpdateIsFarmingSnowdrift();	// snowdrift farming ui visible 갱신
		break;
	case UICategory::SnowdriftFarmDuration:
		localPlayerController->CallDelegateUpdateSnowdriftFarmDuration(farmDuration);	// snowdrift farm duration ui 갱신
		break;
	case UICategory::SelectedItem:
		localPlayerController->CallDelegateUpdateSelectedItem();
		break;
	case UICategory::AllOfUI:
		localPlayerController->CallDelegateUpdateAllOfUI();	// 모든 캐릭터 ui 갱신
		break;
	default:
		break;
	}
}

void AMyCharacter::SelectMatch()
{
	iSelectedItem = ItemTypeList::Match;
	UpdateUI(UICategory::SelectedItem);
}

void AMyCharacter::SelectUmbrella()
{
	iSelectedItem = ItemTypeList::Umbrella;
	UpdateUI(UICategory::SelectedItem);
}

void AMyCharacter::UseSelectedItem()
{
	if (bIsSnowman) return;	// 눈사람은 아이템 사용 x

	switch (iSelectedItem) {
	case ItemTypeList::Match: {	// 성냥 아이템 사용 시
		if (iCurrentMatchCount <= 0) return;	// 보유한 성냥이 없는 경우 리턴
		// 체력 += 20 (체온으로는 2.0도)
		AMyPlayerController* _PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
		if (iSessionId == _PlayerController->iSessionId)
		{
			_PlayerController->SendPlayerInfo(COMMAND_MATCH);
		}
		iCurrentMatchCount -= 1;
		UpdateUI(UICategory::CurMatch);
		break;
	}
	case ItemTypeList::Umbrella:	// 우산 아이템 사용 시
		break;
	default:
		break;
	}
}

void AMyCharacter::UpdateZByTornado()
{	// 캐릭터가 토네이도 내부인 경우 z값 증가
	if (bIsInTornado)
	{
		LaunchCharacter(FVector(0.0f, 0.0f, 20.0f), true, false);
	}
}

void AMyCharacter::UpdateControllerRotateByTornado()
{
	if (rotateCont)
	{
		FRotator contRot = localPlayerController->GetControlRotation();
		FRotator newContRot = FRotator(contRot.Pitch, contRot.Yaw + 5.0f, contRot.Roll);
		localPlayerController->SetControlRotation(newContRot);
	}
}

void AMyCharacter::AttackShotgun()
{
	myAnim->PlayAttackShotgunMontage();

	// 디버깅용 - 실제는 주석 해제
	//iCurrentSnowballCount -= 4;	// 공격 시 눈덩이 소유량 4 감소
	UpdateUI(UICategory::CurSnowball);

}

void AMyCharacter::ChangeWeapon()
{
	iSelectedWeapon = (iSelectedWeapon + 1) % iNumOfWeapons;
}

void AMyCharacter::ShowShotgun()
{
	shotgunMeshComponent->SetVisibility(true);
}

void AMyCharacter::HideShotgun()
{
	shotgunMeshComponent->SetVisibility(false);
}

void AMyCharacter::SpawnSnowballBomb()
{
	if (shotgunProjectileClass)
	{
		UWorld* World = GetWorld();

		if (World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();

			FTransform Muzzle1SocketTransform = shotgunMeshComponent->GetSocketTransform(TEXT("Muzzle1Socket"));
			AMySnowball* snowballBomb1 = World->SpawnActor<AMySnowball>(shotgunProjectileClass, Muzzle1SocketTransform, SpawnParams);

			FTransform Muzzle2SocketTransform = shotgunMeshComponent->GetSocketTransform(TEXT("Muzzle2Socket"));
			AMySnowball* snowballBomb2 = World->SpawnActor<AMySnowball>(shotgunProjectileClass, Muzzle2SocketTransform, SpawnParams);
			FTransform Muzzle3SocketTransform = shotgunMeshComponent->GetSocketTransform(TEXT("Muzzle3Socket"));
			AMySnowball* snowballBomb3 = World->SpawnActor<AMySnowball>(shotgunProjectileClass, Muzzle3SocketTransform, SpawnParams);
			FTransform Muzzle4SocketTransform = shotgunMeshComponent->GetSocketTransform(TEXT("Muzzle4Socket"));
			AMySnowball* snowballBomb4 = World->SpawnActor<AMySnowball>(shotgunProjectileClass, Muzzle4SocketTransform, SpawnParams);

			//if (snowballBomb1->GetClass()->ImplementsInterface(UI_Throwable::StaticClass()))
			//{
			//	FVector cameraLocation;
			//	FRotator cameraRotation;
			//	GetActorEyesViewPoint(cameraLocation, cameraRotation);
			//	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());

			//	II_Throwable::Execute_Throw(snowball, cameraRotation.Vector());
			//}
		}
	}
}
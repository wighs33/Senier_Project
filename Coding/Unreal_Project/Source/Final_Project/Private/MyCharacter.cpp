// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "MyAnimInstance.h"
#include "MyItem.h"
#include "MyPlayerController.h"
#include "Snowdrift.h"

const int AMyCharacter::iMaxHP = 390;
const int AMyCharacter::iMinHP = 270;
const int iBeginSlowHP = 300;	// ĳ���Ͱ� ���ο� ���°� �Ǳ� �����ϴ� hp
const int iNormalSpeed = 600;	// ĳ���� �⺻ �̵��ӵ�
const int iSlowSpeed = 400;		// ĳ���� ���ο� ���� �̵��ӵ�
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
	//GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AMyCharacter::OnHit);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -74.0f), FRotator(0.0f, -90.0f, 0.0f));
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

	static ConstructorHelpers::FClassFinder<UAnimInstance> BEAR_ANIM(TEXT("/Game/Animations/Bear/BearAnimBP.BearAnimBP_C"));
	if (BEAR_ANIM.Succeeded())
	{
		bearAnim = BEAR_ANIM.Class;
		GetMesh()->SetAnimInstanceClass(BEAR_ANIM.Class);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> SNOWMAN_ANIM(TEXT("/Game/Animations/Snowman/SnowMan_AnimBP.SnowMan_AnimBP_C"));
	if (SNOWMAN_ANIM.Succeeded())
	{
		snowmanAnim = SNOWMAN_ANIM.Class;
	}

	GetCharacterMovement()->JumpZVelocity = 800.0f;
	isAttacking = false;

	projectileClass = AMySnowball::StaticClass();

	snowball = nullptr;
	iSessionID = 0;
	iCurrentHP = iMaxHP;
	iMaxSnowballCount = 3;
	iCurrentSnowballCount = 0; 
	bHasUmbrella = false;
	bHasBag = false;
	iMaxMatchCount = 3;
	iCurrentMatchCount = 0;
	farmingItem = nullptr;
	bIsFarming = false;
	bIsInsideOfBonfire = false;	// �ʱⰪ : true�� �����ؾ���,  ĳ���� �ʱ� ������ġ ��ں� ���ο��� ��

	//fMatchDuration = 3.0f;
	//match = true;

	iCharacterState = CharacterState::Normal;
	GetCharacterMovement()->MaxWalkSpeed = iNormalSpeed;	// ĳ���� �̵��ӵ� ����
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	WaitForStartGame();	// ���ð�
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateFarming(DeltaTime);
	UpdateHP();
	UpdateSpeed();
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
}

bool AMyCharacter::CanSetItem()
{
	//return (nullptr == CurrentItem);
	return 0;
}

void AMyCharacter::SetItem(AMyItem* NewItem)
{
	//MYCHECK(nullptr != NewItem && nullptr == CurrentItem);
	//FName ItemSocket(TEXT("hand_rSocket"));
	//auto CurItem = GetWorld()->SpawnActor<AMyItem>(FVector::ZeroVector, FRotator::ZeroRotator);
	//if (nullptr != NewItem)
	//{
	//	NewItem->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, ItemSocket);
	//	NewItem->SetOwner(this);
	//	CurrentItem = NewItem;
	//}
}

void AMyCharacter::UpDown(float NewAxisValue)
{
	if (NewAxisValue != 0)
	{
		AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
		PlayerController->UpdatePlayerInfo(COMMAND_MOVE);
	}
	AddMovementInput(GetActorForwardVector(), NewAxisValue);
}

void AMyCharacter::LeftRight(float NewAxisValue)
{
	if (NewAxisValue != 0)
	{
		AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetWorld()->GetFirstPlayerController());
		PlayerController->UpdatePlayerInfo(COMMAND_MOVE);
	}
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
	//if (iCurrentSnowballCount <= 0) return;	// �����̸� �����ϰ� ���� ������ ���� x

	myAnim->PlayAttackMontage();
	isAttacking = true;
	// ������ - ������ �ּ� ����
	//iSnowballCount -= 1;	// ���� �� ������ ������ 1 ����

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

	iCurrentHP = FMath::Clamp<int>(iCurrentHP - FinalDamage, iMinHP, iMaxHP);

	MYLOG(Warning, TEXT("Actor : %s took Damage : %f, HP : %d"), *GetName(), FinalDamage, iCurrentHP);

	return FinalDamage;
}

void AMyCharacter::ReleaseSnowball()
{
	if (IsValid(snowball))
	{
		FDetachmentTransformRules dtr = FDetachmentTransformRules(EDetachmentRule::KeepWorld, false);
		snowball->DetachFromActor(dtr);

		if (snowball->GetClass()->ImplementsInterface(UI_Throwable::StaticClass()))
		{
			FVector cameraLocation;
			FRotator cameraRotation;
			GetActorEyesViewPoint(cameraLocation, cameraRotation);

			II_Throwable::Execute_Throw(snowball, cameraRotation.Vector());
			snowball = nullptr;
		}

	}
}

void AMyCharacter::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	//auto MySnowball = Cast<AMySnowball>(OtherActor);

	//if (nullptr != MySnowball)
	//{
	//	MYLOG(Warning, TEXT("snowball hit."));
	//}
}

void AMyCharacter::StartFarming()
{
	if (IsValid(farmingItem))
	{
		if (Cast<ASnowdrift>(farmingItem))
		{
			bIsFarming = true;
		}
		else if (Cast<AItembox>(farmingItem))
		{
			AItembox* itembox = Cast<AItembox>(farmingItem);
			switch (itembox->GetItemboxState())
			{
			case ItemboxState::Closed:
				itembox->SetItemboxState(ItemboxState::Opening);
				break;
			case ItemboxState::Opened:
				// �����۹ڽ����� ���빰 �Ĺֿ� �����ϸ� �����۹ڽ����� ������ ���� (�ڽ��� �״�� ������Ŵ)
				if (GetItem(itembox->GetItemType())) { itembox->DeleteItem(); }
				break;
			default:
				break;
			}
		}
	}
}

bool AMyCharacter::GetItem(int itemType)
{
	switch (itemType) {
	case ItemTypeList::Match:
		return true;
		break;
	case ItemTypeList::Umbrella:
		if (bHasUmbrella) return false;	// ����� ���� ���̸� ��� �Ĺ� ���ϵ���
		bHasUmbrella = true;
		return true;
		break;
	case ItemTypeList::Bag:
		if (bHasBag) return false;	// ������ ���� ���̸� ���� �Ĺ� ���ϵ���
		bHasBag = true;
		// ������ �ִ� ������ ���� (���ɵ� �����ϵ���?)
		return true;
		break;
	default:
		return false;
		break;
	}
}

void AMyCharacter::EndFarming()
{
	if (IsValid(farmingItem))
	{
		if (Cast<ASnowdrift>(farmingItem))
		{	// FŰ�� �� ������ �Ĺ� �� FŰ release �� �� ������ duration �ʱ�ȭ
			ASnowdrift* snowdrift = Cast<ASnowdrift>(farmingItem);
			snowdrift->SetFarmDuration(ASnowdrift::fFarmDurationMax);
			bIsFarming = false;
		}
	}
}

void AMyCharacter::UpdateFarming(float deltaTime)
{
	if (bIsFarming)
	{
		if (IsValid(farmingItem))
		{
			ASnowdrift* snowdrift = Cast<ASnowdrift>(farmingItem);
			if (snowdrift)
			{	// �� ������ duration ��ŭ FŰ�� ������ ������ ĳ������ ������ �߰� 
				float lastFarmDuration = snowdrift->GetFarmDuration();
				float newFarmDuration = lastFarmDuration - deltaTime;
				snowdrift->SetFarmDuration(newFarmDuration);

				if (newFarmDuration <= 0)
				{
					iCurrentSnowballCount += 10;
					iCurrentSnowballCount = iCurrentSnowballCount > iMaxSnowballCount ? iMaxSnowballCount : iCurrentSnowballCount;
					snowdrift->Destroy();
				}
			}
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
	case CharacterState::Normal:
		if (iCurrentHP <= iBeginSlowHP)
		{
			iCharacterState = CharacterState::Slow;
			GetCharacterMovement()->MaxWalkSpeed = iSlowSpeed;
		}
		break;
	case CharacterState::Slow:
		if (iCurrentHP > iBeginSlowHP)
		{
			iCharacterState = CharacterState::Normal;
			GetCharacterMovement()->MaxWalkSpeed = iNormalSpeed;
		}
		break;
	case CharacterState::Snowman:
		break;
	default:
		break;
	}
}

void AMyCharacter::ChangeSnowman()
{
	iCurrentHP = iMinHP;
	GetWorldTimerManager().ClearTimer(temperatureHandle);	// ������ �������̴� ü�� ���� �ڵ鷯 �ʱ�ȭ (ü�� ��ȭ���� �ʵ���)
	myAnim->SetDead();
	GetMesh()->SetSkeletalMesh(snowman);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetAnimInstanceClass(snowmanAnim);

	GetCharacterMovement()->MaxWalkSpeed = iSlowSpeed;	// ������� �̵��ӵ��� ���ο� ������ ĳ���Ϳ� �����ϰ� ����
}

void AMyCharacter::WaitForStartGame()
{
	//Delay �Լ�
	FTimerHandle WaitHandle;
	float WaitTime = 3.0f;
	GetWorld()->GetTimerManager().SetTimer(WaitHandle, FTimerDelegate::CreateLambda([&]()
		{
			UpdateTemperatureState();
		}), WaitTime, false);
}

void AMyCharacter::UpdateTemperatureState()
{
	GetWorldTimerManager().ClearTimer(temperatureHandle);	// ������ �������̴� �ڵ鷯 �ʱ�ȭ
	//if (match)
	//{
	//	GetWorldTimerManager().SetTimer(temperatureHandle, this, &AMyCharacter::UpdateTemperatureByMatch, 1.0f, true);
	//}
	//else
	//{
		if (bIsInsideOfBonfire)
		{	// ��ں� ������ ��� �ʴ� ü�� ���� (�ʴ� ȣ��Ǵ� �����Լ�)
			GetWorldTimerManager().SetTimer(temperatureHandle, FTimerDelegate::CreateLambda([&]()
				{
					iCurrentHP += 10;
					iCurrentHP = iCurrentHP > iMaxHP ? iMaxHP : iCurrentHP;
				}), 1.0f, true);
		}
		else
		{	// ��ں� �ܺ��� ��� �ʴ� ü�� ���� (�ʴ� ȣ��Ǵ� �����Լ�)
			GetWorldTimerManager().SetTimer(temperatureHandle, FTimerDelegate::CreateLambda([&]()
				{
					iCurrentHP -= 1;
				}), 1.0f, true);
		}
	//}
}

//void AMyCharacter::UpdateTemperatureByMatch()
//{
//	fMatchDuration -= 1.0f;
//	iCurrentHP += 1.0f;
//
//	if (fMatchDuration <= 0.0f)
//	{
//		match = false;
//		fMatchDuration = 3.0f;
//		UpdateTemperatureState();
//	}
//}
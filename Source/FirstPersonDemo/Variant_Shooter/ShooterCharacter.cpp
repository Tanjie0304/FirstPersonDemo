// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DamageEvents.h"

AShooterCharacter::AShooterCharacter()
{
	// enable replication
	bReplicates = true;

	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 只在服务器初始化 HP，客户端通过复制获得
	if (HasAuthority())
	{
		CurrentHP = MaxHP;
	}

	// 拿到自己时可以先刷新一次 UI（本地或监听服务器）
	OnDamaged.Broadcast(MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f);
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the re spawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 只有本地控制的那一份角色才往服务器发视角
	if (IsLocallyControlled())
	{
		if (AController* C = GetController())
		{
			ServerSetAimRotation(C->GetControlRotation());
		}
	}
}

void AShooterCharacter::ServerSetAimRotation_Implementation(const FRotator& NewAimRotation)
{
	// 只存在于服务器，用来算武器的射线方向
	AimRotation = NewAimRotation;
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

}

// 切换移动速度
void AShooterCharacter::UpdateBerserkMovement()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	// 注意：这里用的是从父类继承来的 bIsBerserkTime
	MoveComp->MaxWalkSpeed = bIsBerserkTime ? BerserkWalkSpeed : NormalWalkSpeed;
}

// 服务器端设置狂暴状态
void AShooterCharacter::ServerSetBerserk_Implementation(bool bNewBerserk)
{
	bIsBerserkTime = bNewBerserk;
	UpdateBerserkMovement();
}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 不要在客户端尝试本地应用伤害或转发（伤害应由服务器上的 projectile/游戏逻辑执行）
	if (!HasAuthority())
	{
		// 客户端忽略本地 TakeDamage 调用；所有权威性由服务器处理
		return 0.0f;
	}

	// Server-side: ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// 记录最近伤害来源（环境伤害 / 自杀时 EventInstigator 可能为 nullptr）
	LastHitInstigator = EventInstigator;

	// Reduce HP (server authoritative)
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD (server-side broadcast/local effects)
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::OnRep_CurrentHP()
{
	// 客户端：收到服务器同步的 HP，广播给 UI
	OnDamaged.Broadcast(MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f);
}

void AShooterCharacter::DoStartFiring()
{
	if (bInputLocked)
	{
		return;
	}
	// 客户端请求服务器开始开火
	if (!HasAuthority())
	{
		ServerStartFiring();
		return;
	}
	// fire the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	// 客户端请求服务器停止开火
	if (!HasAuthority())
	{
		ServerStopFiring();
		return;
	}
	// stop firing the current weapon
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

// RPC实现
void AShooterCharacter::ServerStartFiring_Implementation()
{
	// 服务器再检查一遍，防止客户端乱发
	if (bInputLocked)
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::ServerStopFiring_Implementation()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1)
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon();
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
    FHitResult OutHit;

    FVector Start;
    FVector End;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (IsLocallyControlled())
    {
        // 本地控制的那一份（例如服务器自己的角色），直接用相机的真实朝向
        const FVector CamLoc = GetFirstPersonCameraComponent()->GetComponentLocation();
        const FVector CamForward = GetFirstPersonCameraComponent()->GetForwardVector();

        Start = CamLoc;
        End   = Start + CamForward * MaxAimDistance;
    }
    else
    {
        // 服务器上“远程玩家”的那一份：用客户端同步上来的 AimRotation
        Start = GetPawnViewLocation();      // 角色“眼睛”位置，服务器这边是可靠的
        const FVector AimDir = AimRotation.Vector();

        End = Start + AimDir * MaxAimDistance;
    }

    GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

    // return either the impact point or the trace end
    return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// NOTE: 只应该在服务器上调用（从 ServerAddWeaponClass 或 GameMode 等）
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::ServerAddWeaponClass_Implementation(TSubclassOf<AShooterWeapon> WeaponClass)
{
	AddWeaponClass(WeaponClass); // 只在服务器真正 Spawn 武器
}


void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// only server should run authoritative death behavior
	if (!HasAuthority())
	{
		return;
	}

	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		// 没有击杀者，则不结算
		if (LastHitInstigator)
		{
			// 击杀者队伍
			const uint8 KillerTeam = GM->GetTeamForController(LastHitInstigator);
			// 受害者队伍
			const uint8 VictimTeam = TeamByte;
			// 交给 GameMode 做偷分逻辑（内部会处理同队击杀 / 分数不足等情况）
			GM->StealScoreOnKill(KillerTeam, VictimTeam);
		}
	}
		
	// 通知所有客户端角色死亡
	MulticastOnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::MulticastOnDeath_Implementation()
{
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls（对拥有这个 Pawn 的 PlayerController 有实际效果）
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// 调用蓝图事件播放死亡动画 / 特效 / 切摄像机等
	BP_OnDeath();
}

void AShooterCharacter::OnRespawn()
{
	// 只允许服务器执行重生逻辑
	if (!HasAuthority())
	{
		return;
	}
	// destroy the character to force the PC to respawn (server-side)
	Destroy();
}

void AShooterCharacter::Heal(float Amount)
{
	// 只允许服务器修改血量
	if (!HasAuthority() || Amount <= 0.0f)
	{
		return;
	}

	// 已经死了就不回血
	if (CurrentHP <= 0.0f)
	{
		return;
	}

	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.0f, MaxHP);

	// 服务器本地先广播一次，更新本地 UI
	OnDamaged.Broadcast(MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f);

	// 之后 CurrentHP 会通过复制到客户端，客户端的 OnRep_CurrentHP 再更新各自 UI
}

void AShooterCharacter::HealToFull()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentHP <= 0.0f)
	{
		return;
	}

	CurrentHP = MaxHP;

	// 满血，直接广播 1.0
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterCharacter, CurrentHP);
	DOREPLIFETIME(AShooterCharacter, TeamByte);
}
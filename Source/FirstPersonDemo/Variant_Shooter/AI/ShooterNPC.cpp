// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/AI/ShooterNPC.h"
#include "ShooterWeapon.h"
#include "Variant_Shooter/ShooterGameState.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterGameMode.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"


AShooterNPC::AShooterNPC()
{
	bReplicates = true;          // 这个 Actor 会在网络上传播
	SetReplicateMovement(true);  // 位置 / 旋转通过网络同步
}

void AShooterNPC::BeginPlay()
{
	Super::BeginPlay();

	// 记录默认移动速度，解锁时用来恢复
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultWalkSpeed = MoveComp->MaxWalkSpeed;
	}

	// 订阅 GameState 的“预备倒计时更新”事件
	if (UWorld* World = GetWorld())
	{
		if (AShooterGameState* GS = World->GetGameState<AShooterGameState>())
		{
			GS->OnPreGameCountdownUpdated.AddDynamic(this, &AShooterNPC::OnPreGameCountdownUpdated);
			GS->OnGameOver.AddDynamic(this, &AShooterNPC::OnGameOver);

			// 如果 NPC 生成时倒计时已经在进行了，先同步一次当前值
			OnPreGameCountdownUpdated(GS->PreGameCountTime);
		}
	}

	// spawn the weapon
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Weapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);
}

void AShooterNPC::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterNPC, CurrentHP);
	DOREPLIFETIME(AShooterNPC, bIsDead);
}

void AShooterNPC::OnRep_CurrentHP()
{
	// 这里可以做受击特效 / 播个动画，暂时可以留空
	// 比如：当 CurrentHP <= 0 时在客户端本地再触发一下死亡表现
}

void AShooterNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the death timer
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
}

float AShooterNPC::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 只允许服务器修改血量
	if (!HasAuthority() || Damage <= 0.0f)
	{
		return 0.0f;
	}

	// 已经死亡就不再处理
	if (bIsDead)
	{
		return 0.0f;
	}

	// 记录最后一次对 NPC 造成伤害的控制器（用于结算击杀加分）
	LastHitInstigator = EventInstigator;

	// 扣血（服务器权威）
	CurrentHP -= Damage;

	// 血量归零判断
	if (CurrentHP <= 0.0f)
	{
		CurrentHP = 0.0f;
		Die();
	}

	return Damage;
}

void AShooterNPC::OnPreGameCountdownUpdated(int32 NewTime)
{
	// 真正控制 AI 的是服务端，这里只让服务端做事
	if (!HasAuthority())
	{
		return;
	}

	const bool bShouldLock = (NewTime > 0);
	bAIInputLocked = bShouldLock;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (bShouldLock)
		{
			// 预备阶段：停住并禁止移动
			MoveComp->StopMovementImmediately();
			MoveComp->DisableMovement();
		}
		else if (!bIsDead)
		{
			// 倒计时结束：恢复正常移动
			MoveComp->SetMovementMode(MOVE_Walking);

			if (DefaultWalkSpeed > 0.0f)
			{
				MoveComp->MaxWalkSpeed = DefaultWalkSpeed;
			}
		}
	}

	// 锁定时如果正在射击，立刻停火
	if (bShouldLock && bIsShooting)
	{
		StopShooting();
	}

	// 打一点日志，方便在输出窗口里看到时序
	//UE_LOG(LogTemp, Log, TEXT("NPC %s PreGameCountdown = %d, bAIInputLocked = %s"),
	//	*GetName(),
	//	NewTime,
	//	bAIInputLocked ? TEXT("true") : TEXT("false"));
}

void AShooterNPC::OnGameOver()
{
	// 真正控制 AI 的是服务器
	if (!HasAuthority())
	{
		return;
	}

	// 标记锁定
	bAIInputLocked = true;

	// 停止移动并禁用移动能力
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// 如果还在射击，立刻停火
	if (bIsShooting && Weapon)
	{
		StopShooting();
	}

	// 停止 AI 行为逻辑（StateTree / 行为树等）
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
		{
			BrainComp->StopLogic(TEXT("GameOver"));
		}
	}
}



void AShooterNPC::AttachWeaponMeshes(AShooterWeapon* WeaponToAttach)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	WeaponToAttach->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	WeaponToAttach->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	WeaponToAttach->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
}

void AShooterNPC::PlayFiringMontage(UAnimMontage* Montage)
{
	// unused
}

void AShooterNPC::AddWeaponRecoil(float Recoil)
{
	// unused
}

void AShooterNPC::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	// unused
}

FVector AShooterNPC::GetWeaponTargetLocation()
{
	// start aiming from the camera location
	const FVector AimSource = GetFirstPersonCameraComponent()->GetComponentLocation();

	FVector AimDir, AimTarget = FVector::ZeroVector;

	// do we have an aim target?
	if (CurrentAimTarget)
	{
		// target the actor location
		AimTarget = CurrentAimTarget->GetActorLocation();

		// apply a vertical offset to target head/feet
		AimTarget.Z += FMath::RandRange(MinAimOffsetZ, MaxAimOffsetZ);

		// get the aim direction and apply randomness in a cone
		AimDir = (AimTarget - AimSource).GetSafeNormal();
		AimDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(AimDir, AimVarianceHalfAngle);

		
	} else {

		// no aim target, so just use the camera facing
		AimDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetFirstPersonCameraComponent()->GetForwardVector(), AimVarianceHalfAngle);

	}

	// calculate the unobstructed aim target location
	AimTarget = AimSource + (AimDir * AimRange);

	// run a visibility trace to see if there's obstructions
	FHitResult OutHit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, AimSource, AimTarget, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterNPC::AddWeaponClass(const TSubclassOf<AShooterWeapon>& InWeaponClass)
{
	// unused
}

void AShooterNPC::OnWeaponActivated(AShooterWeapon* InWeapon)
{
	// unused
}

void AShooterNPC::OnWeaponDeactivated(AShooterWeapon* InWeapon)
{
	// unused
}

void AShooterNPC::OnSemiWeaponRefire()
{
	// are we still shooting?
	if (bIsShooting)
	{
		// fire the weapon
		Weapon->StartFiring();
	}
}

void AShooterNPC::Die()
{
	// 只允许服务器执行死亡逻辑
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;

	// ====== 这里给击杀者所在队伍 +20 分 ======
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (LastHitInstigator)
		{
			// 通过 GameMode 统一的队伍接口拿击杀者队伍
			const uint8 KillerTeam = GM->GetTeamForController(LastHitInstigator);

			// 给这个队伍加 20 分
			GM->IncrementTeamScore(KillerTeam, 20);
		}
	}

	// ====== 在所有端同步死亡表现（布娃娃、停运动等） ======
	MulticastOnDeath();

	// 通知服务器上挂着这个委托的 Listener（如果你在别处用到了 OnPawnDeath）
	OnPawnDeath.Broadcast();

	// 安排延迟销毁（只需服务器调用，客户端会跟着销毁）
	GetWorld()->GetTimerManager().SetTimer(
		DeathTimer,
		this,
		&AShooterNPC::DeferredDestruction,
		DeferredDestructionTime,
		false
	);
}

void AShooterNPC::MulticastOnDeath_Implementation()
{
	// 禁用胶囊碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 停止移动
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->StopActiveMovement();
	}

	// 启用第三人称 Mesh 的布娃娃物理
	GetMesh()->SetCollisionProfileName(RagdollCollisionProfile);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetPhysicsBlendWeight(1.0f);
}

void AShooterNPC::DeferredDestruction()
{
	Destroy();
}

void AShooterNPC::StartShooting(AActor* ActorToShoot)
{
	// 预备阶段 / 已死亡 / 没武器 时不允许开火
	if (bAIInputLocked || bIsDead || !Weapon)
	{
		return;
	}

	// 保存当前瞄准目标
	CurrentAimTarget = ActorToShoot;

	// 标记正在射击
	bIsShooting = true;

	// 通知武器开始开火
	Weapon->StartFiring();
}

void AShooterNPC::StopShooting()
{
	// lower the flag
	bIsShooting = false;

	// signal the weapon
	Weapon->StopFiring();
}

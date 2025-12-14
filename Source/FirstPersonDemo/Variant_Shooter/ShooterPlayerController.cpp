// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterPlayerController.h"
#include "Variant_Shooter/ShooterGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterCharacter.h"
#include "ShooterBulletCounterUI.h"
#include "ShooterUI.h" 
#include "FirstPersonDemo.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController())
	{
		if (SVirtualJoystick::ShouldDisplayTouchInterface())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			}
			else {

				UE_LOG(LogFirstPersonDemo, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}

		// create the bullet counter widget and add it to the screen
		BulletCounterUI = CreateWidget<UShooterBulletCounterUI>(this, BulletCounterUIClass);

		if (BulletCounterUI)
		{
			BulletCounterUI->AddToPlayerScreen(0);
		}
		else {

			UE_LOG(LogFirstPersonDemo, Error, TEXT("Could not spawn bullet counter widget."));

		}

		// === 新增：在本地创建 Shooter UI 并绑定到 GameState 的分数更新 ===
		if (ShooterUIClass)
		{
			ShooterUI = CreateWidget<UShooterUI>(this, ShooterUIClass);
			
			if (ShooterUI)
			{
				ShooterUI->AddToViewport(0);
			}
			else
			{
				UE_LOG(LogFirstPersonDemo, Error, TEXT("Could not spawn ShooterUI widget."));
			}
		}

		// 绑定 GameState 的分数与倒计时广播（客户端接收）
		if (GetWorld())
		{
			if (AShooterGameState* GS = GetWorld()->GetGameState<AShooterGameState>())
			{
				GS->OnTeamScoreUpdated.AddDynamic(this, &AShooterPlayerController::OnTeamScoreUpdated);
				// 绑定游戏结束倒计时更新
				GS->OnCountdownUpdated.AddDynamic(this, &AShooterPlayerController::OnCountdownUpdated);

				// 绑定游戏开始倒计时更新
				GS->OnPreGameCountdownUpdated.AddDynamic(this, &AShooterPlayerController::OnPreGameCountdownUpdated);

				// 绑定游戏结束事件
				GS->OnGameOver.AddDynamic(this, &AShooterPlayerController::OnGameOver); 

				// 若 GameState 已有当前倒计时值（例如玩家在中途加入），主动更新一次本地 UI
				if (ShooterUI)
				{
					// 预游戏倒计时
					if (GS->PreGameCountTime > 0)
					{
						ShooterUI->BP_UpdatePreGameCountdown(static_cast<float>(GS->PreGameCountTime));
					}
					else
					{
						ShooterUI->BP_UpdateCountdown(static_cast<float>(GS->GameCountTime));
					}
				}

				// 用当前的 PreGameCountTime 主动同步一次输入锁状态
				OnPreGameCountdownUpdated(GS->PreGameCountTime);
			}
		}
	}

}

void AShooterPlayerController::SetupInputComponent()
{
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	InitializePawn(InPawn);

	// 只对本地控制器处理输入模式
	if (IsLocalPlayerController())
	{
		// 每次获得 Pawn，都强制切回 FPS 模式
		FInputModeGameOnly GameInputMode;
		SetInputMode(GameInputMode);

		bShowMouseCursor = false;
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
	}
}

void AShooterPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	InitializePawn(GetPawn());   //  客户端这儿才会被调用

	if (IsLocalPlayerController())
	{
		FInputModeGameOnly GameInputMode;
		SetInputMode(GameInputMode);

		bShowMouseCursor = false;
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
	}
}

void AShooterPlayerController::InitializePawn(APawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &AShooterPlayerController::OnPawnDestroyed);

	// is this a shooter character?
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn))
	{

		// 服务器为新 Pawn 应用队伍 ID
		if (HasAuthority())
		{
			if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
			{
				ShooterCharacter->TeamByte = TeamId;
			}
		}

		// add the player tag（客户端/服务端加都无所谓）
		ShooterCharacter->Tags.Add(PlayerPawnTag);

		// subscribe to the pawn's delegates
		ShooterCharacter->OnBulletCountUpdated.AddDynamic(this, &AShooterPlayerController::OnBulletCountUpdated);
		ShooterCharacter->OnDamaged.AddDynamic(this, &AShooterPlayerController::OnPawnDamaged);

		// force update the life bar（先给 1.0，之后 RepNotify 会同步真实血量）
		ShooterCharacter->OnDamaged.Broadcast(1.0f);
	}
}

void AShooterPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// 所有端都可以先把本地 UI 置 0
	if (BulletCounterUI)
	{
		BulletCounterUI->BP_UpdateBulletCounter(0, 0);
	}

	// 只有服务器负责真正的重生逻辑
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GM = World->GetAuthGameMode<AGameModeBase>())
		{
			// 交给 GameMode，用我们刚刚写的 ChoosePlayerStart_Implementation 来选择出生点
			GM->RestartPlayer(this);
		}
	}
}

void AShooterPlayerController::OnBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	// update the UI
	if (BulletCounterUI)
	{
		BulletCounterUI->BP_UpdateBulletCounter(MagazineSize, Bullets);
	}
}

void AShooterPlayerController::OnPawnDamaged(float LifePercent)
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_Damaged(LifePercent);
	}
}

void AShooterPlayerController::OnTeamScoreUpdated(uint8 Team, int32 Score)
{
	// 当客户端接收到分数更新，更新本地 UI（Blueprint exposed function）
	if (ShooterUI)
	{
		ShooterUI->BP_UpdateScore(Team, Score);
	}
}

void AShooterPlayerController::OnCountdownUpdated(int32 NewTime)
{
	// 1）正常更新倒计时数字
	if (ShooterUI)
	{
		// BP 接口本身就是 int32，就直接传
		ShooterUI->BP_UpdateCountdown(NewTime);
	}

	// 只对本地玩家做下面逻辑
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 2）根据剩余时间计算当前是否“狂暴时间”
	const bool bIsNowBerserk = (NewTime <= 10 && NewTime > 0);

	// 3）UI：检测状态边沿变化
	if (ShooterUI)
	{
		// 非狂暴 -> 狂暴 ：刚进入狂暴
		if (bIsNowBerserk && !bWasInBerserk)
		{
			// 屏幕中央显示“狂暴倒计时！” 1 秒
			ShooterUI->BP_ShowBerserkHint();

			// UI 边框变红
			ShooterUI->BP_OnBerserkStateChanged(true);
		}
		// 狂暴 -> 非狂暴 ：退出狂暴
		else if (!bIsNowBerserk && bWasInBerserk)
		{
			// UI 边框恢复正常
			ShooterUI->BP_OnBerserkStateChanged(false);
		}
	}

	// 4）记住这一次的状态，下次对比
	bWasInBerserk = bIsNowBerserk;

	// 5）通知服务器切换角色的狂暴状态（加速移动）
	APawn* Pa = GetPawn();
	if (AShooterCharacter* Cha = Cast<AShooterCharacter>(Pa))
	{
		Cha->ServerSetBerserk(bIsNowBerserk);
	}
}


// 单独处理赛前倒计时（锁输入、显示 GO、解锁等本地行为）
void AShooterPlayerController::OnPreGameCountdownUpdated(int32 NewTime)
{
	// 更新 HUD
	if (ShooterUI)
	{
		ShooterUI->BP_UpdatePreGameCountdown(static_cast<float>(NewTime));
	}

	// 仅对本地玩家进行输入锁定/解锁
	if (!IsLocalPlayerController()) return;

	APawn* Pa = GetPawn();
	if (!Pa) return;

	if (AShooterCharacter* Cha = Cast<AShooterCharacter>(Pa))
	{
		if (NewTime > 0)
		{
			// 预游戏倒计时期间锁输入
			Cha->bInputLocked = true;
		}
		else
		{
			// 倒计时结束：解锁输入
			Cha->bInputLocked = false;
		}
	}
}

void AShooterPlayerController::OnGameOver()
{
	// 只在本地玩家上做 UI 和输入处理
	if (!IsLocalPlayerController())
	{
		return;
	}

	int32 MyScore = 0;
	int32 OtherScore = 0;

	// 1）从 GameState 中拿到自己队伍的分数和另一个队伍的分数
	if (UWorld* World = GetWorld())
	{
		if (AShooterGameState* GS = World->GetGameState<AShooterGameState>())
		{
			// 自己队伍
			MyScore = GS->GetScoreForTeam(TeamId);

			// 假设是 2 队模式，找到一个“不是自己队伍”的分数
			for (const FTeamScore& TS : GS->TeamScores)
			{
				if (TS.TeamId != TeamId)
				{
					OtherScore = TS.Score;
					break;
				}
			}
		}
	}

	const bool bIsWin = (MyScore >= OtherScore);

	// 2）通知 UI 在屏幕中央显示结算
	if (ShooterUI)
	{
		ShooterUI->BP_OnGameOver(bIsWin, MyScore, OtherScore);
	}

	// 2.5）切到 UI-only 模式 + 显示鼠标
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	if (ShooterUI)
	{
		InputMode.SetWidgetToFocus(ShooterUI->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// 3）锁定玩家移动和视角（再次把 bInputLocked 设为 true）
	APawn* Pa = GetPawn();
	if (AShooterCharacter* Cha = Cast<AShooterCharacter>(Pa))
	{
		Cha->bInputLocked = true;

		// 停止当前移动
		if (UCharacterMovementComponent* MoveComp = Cha->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}

		// 如果还在射击，让服务器那边停火
		Cha->DoStopFiring();
	}
}



void AShooterPlayerController::RequestRestartGame()
{
	// 只允许本地玩家发起
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 发 RPC 给服务器
	ServerRestartGame();
}

void AShooterPlayerController::ServerRestartGame_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AShooterGameMode* GM = World->GetAuthGameMode<AShooterGameMode>())
		{
			GM->RestartGameForAll();
		}
	}
}


void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, TeamId);
}

void AShooterPlayerController::RequestLevelTransition(FString MapName)
{
	// 1. 本地检查：防止空名字
	if (MapName.IsEmpty())
	{
		return;
	}

	// 2. 发起请求：
	// - 如果是客户端调用：这会发送网络包给服务器。
	// - 如果是服务端调用：这会直接在本地执行 Implementation。
	ServerRequestLevelTransition(MapName);
}

void AShooterPlayerController::ServerRequestLevelTransition_Implementation(const FString& MapName)
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		// 拼接 ?listen 参数，确保跳转后依然是多人游戏主机
		// 如果不加 ?listen，服务器跳转后会变成单机模式，把所有客户端踢掉
		FString TravelURL = MapName + TEXT("?listen");

		// 执行无缝漫游，带走所有已连接的客户端
		World->ServerTravel(TravelURL);
	}
}
// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "Variant_Shooter/ShooterGameState.h"
#include "Variant_Shooter/ShooterPlayerController.h"
#include "ShooterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AShooterGameMode::AShooterGameMode()
{
	// use our custom GameState class
	GameStateClass = AShooterGameState::StaticClass();
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
	check(GEngine != nullptr);

	// 开始预游戏倒计时（仅在服务端）
	if (HasAuthority())
	{
		StartPreGameCountdown();
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Welcome to First111 Person Demo!"));

	if (HasAuthority())
	{
		AShooterGameState* GS = GetGameState<AShooterGameState>();
		if (GS)
		{
			for (uint8 Team = 0; Team < NumTeams; ++Team)
			{
				// 这里用 IncrementTeamScore(Team, 0) 也可以，
				// 它内部会调用 Server_SetTeamScore，进而在 TeamScores 里创建条目并广播 UI。
				IncrementTeamScore(Team, 0);
			}
		}
	}
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte, int32 AddScore)
{
	// 获取 GameState 并在服务端更新分数
	AShooterGameState* GS = GetGameState<AShooterGameState>();
	if (GS && HasAuthority())
	{
		int32 Score = GS->GetScoreForTeam(TeamByte);
		Score += AddScore;

		// 更新 GameState（Server_SetTeamScore 会设置 TeamScores 并同步 LastUpdatedX）
		GS->Server_SetTeamScore(TeamByte, Score);
	}
}

uint8 AShooterGameMode::GetTeamForController(AController* Controller) const
{
	// 先看是不是我们的 ShooterPlayerController
	if (AShooterPlayerController* SPC = Cast<AShooterPlayerController>(Controller))
	{
		return SPC->TeamId;
	}

	// 兜底：用原来的 TMap
	if (const uint8* Found = PlayerTeams.Find(Controller))
	{
		return *Found;
	}

	// 实在没找到，就默认 0
	return 0;
}

void AShooterGameMode::PostLogin(APlayerController* NewPlayer)
{
	if (HasAuthority())
	{
		const uint8 Team = AssignTeamForController(NewPlayer);

		//UE_LOG(LogTemp, Log, TEXT("PostLogin: %s final Team=%d"),
		//	*GetNameSafe(NewPlayer), Team);
	}

	Super::PostLogin(NewPlayer);
}

uint8 AShooterGameMode::AssignTeamForController(AController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return 0;
	}

	// 1）如果已经有记录，就直接返回，不重复分队
	if (uint8* Found = PlayerTeams.Find(Controller))
	{
		return *Found;
	}

	// 2）没有记录，就分一个新队伍
	uint8 AssignedTeam = NextTeamId;
	NextTeamId = (NextTeamId + 1) % NumTeams;

	PlayerTeams.Add(Controller, AssignedTeam);

	if (AShooterPlayerController* SPC = Cast<AShooterPlayerController>(Controller))
	{
		SPC->TeamId = AssignedTeam;
		//UE_LOG(LogTemp, Log, TEXT("[AssignTeam] %s -> Team %d (SPC->TeamId=%d)"),
		//	*GetNameSafe(SPC), AssignedTeam, SPC->TeamId);
	}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("[AssignTeam] Controller %s is not ShooterPlayerController, Team=%d"),
	//		*GetNameSafe(Controller), AssignedTeam);
	//}

	return AssignedTeam;
}


void AShooterGameMode::Logout(AController* Exiting)
{
	if (HasAuthority())
	{
		PlayerTeams.Remove(Exiting);
	}

	Super::Logout(Exiting);
}

void AShooterGameMode::StartPreGameCountdown()
{
	// 每秒递减 PreGameTime，并更新 GameState
	GetWorldTimerManager().SetTimer(PreGameTimerHandle, this, &AShooterGameMode::AdvancePreGameTimer, 1.0f, true);
	// 立即把初始值写入 GameState，确保加入玩家能立刻看到
	if (AShooterGameState* GS = GetGameState<AShooterGameState>())
	{
		GS->Server_SetPreGameCountTime(PreGameTime);
	}
}

void AShooterGameMode::AdvancePreGameTimer()
{
	if (!HasAuthority()) return;

	--PreGameTime;

	if (AShooterGameState* GS = GetGameState<AShooterGameState>())
	{
		GS->Server_SetPreGameCountTime(PreGameTime);
	}

	if (PreGameTime < 1)
	{
		// stop pre-game timer
		GetWorldTimerManager().ClearTimer(PreGameTimerHandle);

		// 赛前倒计时结束后，开始正式游戏倒计时（或直接解锁玩家）
		StartGameCountdown();
	}
}

void AShooterGameMode::StartGameCountdown()
{
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &AShooterGameMode::AdvanceGameTimer, 1.0f, true);
}

void AShooterGameMode::AdvanceGameTimer()
{
	--GameCountTime;
	// 在服务端更新 GameState（GameState 会复制到客户端并触发 OnRep）
	AShooterGameState* GS = GetGameState<AShooterGameState>();
	if (GS && HasAuthority())
	{
		GS->Server_SetGameCountTime(GameCountTime);
	}

	if (GameCountTime < 1)
	{
		// stop the timer
		GetWorldTimerManager().ClearTimer(GameTimerHandle);

		// 设置 GameOver（同步到所有客户端）
		if (GS && HasAuthority())
		{
			GS->Server_SetGameOver(true);
		}

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Game Over!"));
	}
}

void AShooterGameMode::StealScoreOnKill(uint8 KillerTeam, uint8 VictimTeam)
{
	if (!HasAuthority())
	{
		return;
	}

	// 同队 / 未分队 等情况不处理
	if (KillerTeam == VictimTeam)
	{
		return;
	}

	AShooterGameState* GS = GetGameState<AShooterGameState>();
	if (!GS)
	{
		return;
	}

	// 先检查被击杀者队伍当前分数
	const int32 VictimScore = GS->GetScoreForTeam(VictimTeam);

	// 取一般（向下取整）
	const int32 StealAmount = VictimScore / 2;

	if (StealAmount <= 0)
	{
		// 对方没分数可偷
		return;
	}

	// 击杀者队伍加分
	IncrementTeamScore(KillerTeam, StealAmount);

	// 被击杀者队伍扣分
	IncrementTeamScore(VictimTeam, -StealAmount); // 这里不会是负数
}


AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// 先确保有队伍，没有就分一个
	const uint8 TeamId = AssignTeamForController(Player);

	uint8 TeamIdFromController = 0;
	if (AShooterPlayerController* SPC = Cast<AShooterPlayerController>(Player))
	{
		TeamIdFromController = SPC->TeamId;
	}

	//UE_LOG(LogTemp, Log, TEXT("ChoosePlayerStart: Player=%s, TeamId=%d, SPC->TeamId=%d"),
	//	*GetNameSafe(Player),
	//	TeamId,
	//	TeamIdFromController);

	// 按队伍选择不同 Tag
	FName DesiredTag = NAME_None;
	if (TeamId == 0)
	{
		DesiredTag = FName(TEXT("Team0"));
	}
	else if (TeamId == 1)
	{
		DesiredTag = FName(TEXT("Team1"));
	}

	if (DesiredTag.IsNone())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);

	for (AActor* StartActor : Starts)
	{
		if (APlayerStart* PS = Cast<APlayerStart>(StartActor))
		{
			if (PS->PlayerStartTag == DesiredTag)
			{
				//UE_LOG(LogTemp, Log, TEXT("ChoosePlayerStart: %s -> %s (Team %d, Tag %s)"),
				//	*GetNameSafe(Player),
				//	*GetNameSafe(PS),
				//	TeamId,
				//	*DesiredTag.ToString());

				return PS;
			}
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AShooterGameMode::RestartGameForAll()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 取当前关卡名（注意 PIE 情况会有前缀）
	FString MapName = World->GetMapName();          // 比如 "UEDPIE_0_L_ShooterMap"
	FString Prefix = World->StreamingLevelsPrefix; // 比如 "UEDPIE_0_"

	if (!Prefix.IsEmpty() && MapName.StartsWith(Prefix))
	{
		MapName.RightChopInline(Prefix.Len(), false);   // 去掉 PIE 前缀，得到真正的关卡名
	}

	// Listen Server 记得加 ?listen，保持当前是房主
	const FString URL = MapName + TEXT("?listen");

	// 用 ServerTravel，所有客户端都会跟着换图
	World->ServerTravel(URL);
}


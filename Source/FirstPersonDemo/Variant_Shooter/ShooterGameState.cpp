// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Shooter/ShooterGameState.h"
#include "Net/UnrealNetwork.h"

AShooterGameState::AShooterGameState()
{
	LastUpdatedTeam = 0;
	LastUpdatedScore = 0;
	GameCountTime = 0;
	bReplicates = true;
	bIsGameOver = false;
}

int32 AShooterGameState::GetScoreForTeam(uint8 Team) const
{
	for (const FTeamScore& TS : TeamScores)
	{
		if (TS.TeamId == Team)
		{
			return TS.Score;
		}
	}
	return 0;
}

void AShooterGameState::Server_SetTeamScore(uint8 Team, int32 Score)
{
	// 仅在服务器上调用
	for (FTeamScore& TS : TeamScores)
	{
		if (TS.TeamId == Team)
		{
			TS.Score = Score;
			LastUpdatedTeam = Team;
			LastUpdatedScore = Score;
			// 在服务器上也触发（客户端会通过 OnRep_LastUpdatedScore 收到）
			// OnRep_LastUpdatedScore();
			// 多播到所有客户端 + 服务器
			MulticastTeamScoreUpdated(Team, Score);
			return;
		}
	}

	// 如果不存在则新增
	FTeamScore New;
	New.TeamId = Team;
	New.Score = Score;
	TeamScores.Add(New);

	LastUpdatedTeam = Team;
	LastUpdatedScore = Score;
	// OnRep_LastUpdatedScore();
	MulticastTeamScoreUpdated(Team, Score);
}

void AShooterGameState::Server_SetGameCountTime(int32 NewTime)
{
	// 仅在服务器上调用
	GameCountTime = NewTime;

	// 在服务器上也触发（客户端通过 OnRep_GameCountTime 收到）
	OnRep_GameCountTime();
}

void AShooterGameState::Server_SetPreGameCountTime(int32 NewTime)
{
	// 仅在服务器上调用
	PreGameCountTime = NewTime;

	// 在服务器上也触发（客户端通过 OnRep_PreGameCountTime 收到）
	OnRep_PreGameCountTime();
}

void AShooterGameState::Server_SetGameOver(bool bNewGameOver)
{
	// 仅在服务器上调用
	bIsGameOver = bNewGameOver;

	// 在服务器上也手动触发一次（客户端通过 OnRep_GameOver 收到）
	OnRep_GameOver();
}

void AShooterGameState::OnRep_LastUpdatedScore()
{
	// 在客户端执行：广播给绑定者（例如各客户端的 PlayerController）
	OnTeamScoreUpdated.Broadcast(LastUpdatedTeam, LastUpdatedScore);
}

void AShooterGameState::OnRep_GameCountTime()
{
	// 在客户端执行：广播倒计时更新
	OnCountdownUpdated.Broadcast(GameCountTime);
}

void AShooterGameState::OnRep_PreGameCountTime()
{
	// 在客户端执行：广播预游戏倒计时更新
	OnPreGameCountdownUpdated.Broadcast(PreGameCountTime);
}

void AShooterGameState::OnRep_GameOver()
{
	if (bIsGameOver)
	{
		// 在客户端执行：广播“游戏结束”事件
		OnGameOver.Broadcast();
	}
}

void AShooterGameState::MulticastTeamScoreUpdated_Implementation(uint8 Team, int32 NewScore)
{
	// 在所有端（服务器 + 所有客户端）广播
	OnTeamScoreUpdated.Broadcast(Team, NewScore);
}

void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterGameState, TeamScores);
	DOREPLIFETIME(AShooterGameState, LastUpdatedTeam);
	DOREPLIFETIME(AShooterGameState, LastUpdatedScore);
	DOREPLIFETIME(AShooterGameState, GameCountTime);
	DOREPLIFETIME(AShooterGameState, PreGameCountTime);
	DOREPLIFETIME(AShooterGameState, bIsGameOver);
}


// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"


/**
 *  Simple GameMode for a first person shooter game
 *  Manages game UI
 *  Keeps track of team scores
 */

class AShooterGameState;
UCLASS(abstract)
class FIRSTPERSONDEMO_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShooterGameMode();

protected:

	// 游戏倒计时
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameTimer")
	int32 GameCountTime = 60;

	FTimerHandle GameTimerHandle;

	// 游戏开始倒计时
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameTimer")
	int32 PreGameTime = 3;

	FTimerHandle PreGameTimerHandle;

	// ==== 队伍相关 ====
	// 队伍数量（默认2队）
	UPROPERTY(EditDefaultsOnly, Category="Teams")
    uint8 NumTeams = 2;

    // 记录每个 Controller 所属队伍（只在服务器用）
    UPROPERTY()
    TMap<AController*, uint8> PlayerTeams; 

	// 简单轮换分配用的下一个队伍 ID
	uint8 NextTeamId = 0;

	// 玩家加入/离开时处理队伍
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// 确保 Controller 有队伍，没有就分配一个并返回
	uint8 AssignTeamForController(AController* Controller);

public:

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte, int32 AddScore);

	/** Killer steals half of victim team's score (server-side only) */
	void StealScoreOnKill(uint8 KillerTeam, uint8 VictimTeam);

	// 游戏结束倒计时
	void StartGameCountdown();
	void AdvanceGameTimer();

	// 预游戏倒计时
	void StartPreGameCountdown();
	void AdvancePreGameTimer();

	// 服务器统一重开当前关卡
	UFUNCTION(BlueprintCallable, Category = "Shooter|Game")
	void RestartGameForAll();

	// 查询某个 Controller 的队伍（没找到就返回 0）
	uint8 GetTeamForController(AController* Controller) const;
};

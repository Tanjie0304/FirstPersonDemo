// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShooterGameState.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamScoreUpdated, uint8, Team, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownUpdated, int32, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPreGameCountdownUpdated, int32, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameOver);

USTRUCT(BlueprintType)
struct FTeamScore
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Teams")
	uint8 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Teams")
	int32 Score = 0;
};

UCLASS()
class FIRSTPERSONDEMO_API AShooterGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AShooterGameState();

	// 使用数组替代不可复制的 map
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Teams")
	TArray<FTeamScore> TeamScores;

	// 最近一次变更的队伍与分数，用于触发 OnRep 通知（只复制最近一次变更）
	UPROPERTY(ReplicatedUsing = OnRep_LastUpdatedScore)
	uint8 LastUpdatedTeam;

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdatedScore)
	int32 LastUpdatedScore;

	// 倒计时（由服务端维护并复制到客户端）
	UPROPERTY(ReplicatedUsing = OnRep_GameCountTime)
	int32 GameCountTime;

	// 预游戏倒计时
	UPROPERTY(ReplicatedUsing = OnRep_PreGameCountTime)
	int32 PreGameCountTime;

	// 分数更新广播（客户端订阅）
	UPROPERTY(BlueprintAssignable)
	FOnTeamScoreUpdated OnTeamScoreUpdated;

	// 倒计时更新广播（客户端订阅）
	UPROPERTY(BlueprintAssignable)
	FOnCountdownUpdated OnCountdownUpdated;

	// 预游戏倒计时更新广播（客户端订阅）
	UPROPERTY(BlueprintAssignable)
	FOnPreGameCountdownUpdated OnPreGameCountdownUpdated;

	// 新增：多播一个“某队分数更新”的通知
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTeamScoreUpdated(uint8 Team, int32 NewScore);

	// 游戏是否结束
	UPROPERTY(ReplicatedUsing = OnRep_GameOver)
	bool bIsGameOver = false;

	// 游戏结束广播（客户端订阅）
	UPROPERTY(BlueprintAssignable)
	FOnGameOver OnGameOver;

	// 仅由服务器调用以更新分数
	void Server_SetTeamScore(uint8 Team, int32 Score);

	// 查询某队当前分数（只读）
	int32 GetScoreForTeam(uint8 Team) const;

	// 仅由服务器调用以更新倒计时
	void Server_SetGameCountTime(int32 NewTime);

	// 仅由服务器调用以更新预游戏倒计时
	void Server_SetPreGameCountTime(int32 NewTime);

	// 服务器设置Gameover状态
	void Server_SetGameOver(bool bNewGameOver);

protected:
	UFUNCTION()
	void OnRep_LastUpdatedScore();

	UFUNCTION()
	void OnRep_GameCountTime();

	UFUNCTION()
	void OnRep_PreGameCountTime();

	UFUNCTION()
	void OnRep_GameOver();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	
};

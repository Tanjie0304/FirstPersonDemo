// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

#include "Variant_Shooter/ShooterGameState.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;
class AShooterCharacter;
class UShooterBulletCounterUI;
class UShooterUI;

/**
 *  Simple PlayerController for a first person shooter game
 *  Manages input mappings
 *  Respawns the player pawn when it's destroyed
 */
UCLASS(abstract)
class FIRSTPERSONDEMO_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input mapping contexts for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** Character class to respawn when the possessed pawn is destroyed */
	UPROPERTY(EditAnywhere, Category="Shooter|Respawn")
	TSubclassOf<AShooterCharacter> CharacterClass;

	/** Type of bullet counter UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterBulletCounterUI> BulletCounterUIClass;

	/** Tag to grant the possessed pawn to flag it as the player */
	UPROPERTY(EditAnywhere, Category="Shooter|Player")
	FName PlayerPawnTag = FName("Player");

	/** Pointer to the bullet counter UI widget */
	TObjectPtr<UShooterBulletCounterUI> BulletCounterUI;

	/** Type of shooter UI widget to spawn (客户端本地) */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterUI> ShooterUIClass;

	/** Pointer to the shooter UI widget (本地) */
	TObjectPtr<UShooterUI> ShooterUI;

	// 记录上一次是否处于狂暴状态
	bool bWasInBerserk = false;

protected:

	/** Gameplay Initialization */
	virtual void BeginPlay() override;

	/** 当客户端接收到分数更新，更新本地 UI（由 GameState 广播调用） */
	UFUNCTION()
	void OnTeamScoreUpdated(uint8 Team, int32 Score);

	/** 当客户端接收到倒计时更新，更新本地 UI（由 GameState 广播调用） */
	UFUNCTION()
	void OnCountdownUpdated(int32 NewTime);

	/* 游戏开始倒计时 */
	UFUNCTION()
	void OnPreGameCountdownUpdated(int32 NewTime);

	// 游戏结束回调
	UFUNCTION()
	void OnGameOver();

	// 真正在服务器上执行关卡重载
	UFUNCTION(Server, Reliable)
	void ServerRestartGame();

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn) override;

	/* 服务器OnPossess后，客户端调用 */
	virtual void OnRep_Pawn() override;

	void InitializePawn(APawn* InPawn);

	/** Called if the possessed pawn is destroyed */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Called when the bullet count on the possessed pawn is updated */
	UFUNCTION()
	void OnBulletCountUpdated(int32 MagazineSize, int32 Bullets);

	/** Called when the possessed pawn is damaged */
	UFUNCTION()
	void OnPawnDamaged(float LifePercent);

	// 真正在服务器上执行关卡切换
	UFUNCTION(Server, Reliable)
	void ServerRequestLevelTransition(const FString& MapName);

public:
	// 玩家所在队伍（由 GameMode 分配）
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
	uint8 TeamId = 0;

public:
	// 本地 UI 调用：请求重新开始当前关卡
	UFUNCTION(BlueprintCallable, Category = "Shooter|Game")
	void RequestRestartGame();

	// 供蓝图 UI 调用的通用接口用于请求关卡切换
	UFUNCTION(BlueprintCallable, Category = "Shooter|Game")
	void RequestLevelTransition(FString MapName);
};

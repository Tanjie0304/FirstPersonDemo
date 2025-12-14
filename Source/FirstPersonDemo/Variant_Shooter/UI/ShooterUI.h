// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterUI.generated.h"

/**
 *  Simple scoreboard UI for a first person shooter game
 */
UCLASS(abstract)
class FIRSTPERSONDEMO_API UShooterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Allows Blueprint to update score sub-widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Score"))
	void BP_UpdateScore(uint8 TeamByte, int32 Score);

	// 实现游戏时间倒计时显示
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "Update Game Timer"))
	void BP_UpdateCountdown(int32 RemainingTime);

	// 实现游戏开始倒计时显示
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "Update Pre-Game Timer"))
	void BP_UpdatePreGameCountdown(int32 RemainingTime);

	// 狂暴状态切换（用于改边框颜色）
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "On Berserk State Changed"))
	void BP_OnBerserkStateChanged(bool bIsBerserk);

	// 刚进入狂暴时的 1 秒提示
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "Show Berserk Hint"))
	void BP_ShowBerserkHint();

	// 游戏结束结算界面显示
	UFUNCTION(BlueprintImplementableEvent, Category = "Shooter", meta = (DisplayName = "On Game Over"))
	void BP_OnGameOver(bool bIsWin, int32 LocalScore, int32 OtherScore);
};

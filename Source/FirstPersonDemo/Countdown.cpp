// Fill out your copyright notice in the Description page of Project Settings.


#include "Countdown.h"
#include "Components/TextRenderComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Variant_Shooter/ShooterGameState.h"
#include "Variant_Shooter/ShooterGameMode.h"
#include "ShooterCharacter.h"

// Sets default values
ACountdown::ACountdown()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	CountdownText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CountdownNumber"));
	CountdownText->SetHorizontalAlignment(EHTA_Center); // 居中对齐
	CountdownText->SetWorldSize(150.0f); // 设置文本大小
	// 使得文字面向玩家
	CountdownText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	RootComponent = CountdownText;
	CountdownText->SetVisibility(false);
	bReplicates = false;
}

// Called when the game starts or when spawned
void ACountdown::BeginPlay()
{
	Super::BeginPlay();

	// 尝试绑定到游戏状态
	TryBindToGameState();
	
}

// Called every frame
void ACountdown::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACountdown::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HideGoTimerHandle);
	}

	if (AShooterGameState* GS = GetWorld() ? GetWorld()->GetGameState<AShooterGameState>() : nullptr)
	{
		GS->OnPreGameCountdownUpdated.RemoveDynamic(this, &ACountdown::OnPreGameCountdownUpdated);
	}

	Super::EndPlay(EndPlayReason);
}

void ACountdown::TryBindToGameState()
{
	if (!GetWorld()) return;

	if (AShooterGameState* GS = GetWorld()->GetGameState<AShooterGameState>())
	{
		GS->OnPreGameCountdownUpdated.AddDynamic(this, &ACountdown::OnPreGameCountdownUpdated);

		// 如果已有值（玩家中途加入），立即刷新显示
		UpdateTimerDisplay(GS->PreGameCountTime);

		if (GetWorld()->GetTimerManager().IsTimerActive(BindRetryTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		}
	}
	else
	{
		// 每 0.2s 重试绑定一次，直到成功
		if (!GetWorld()->GetTimerManager().IsTimerActive(BindRetryTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(BindRetryTimerHandle, this, &ACountdown::TryBindToGameState, 0.2f, true);
		}
	}
}

void ACountdown::OnPreGameCountdownUpdated(int32 NewTime)
{
	if (NewTime > 0)
	{
		UpdateTimerDisplay(NewTime);

		// 锁定本地玩家输入
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (AShooterCharacter* Character = Cast<AShooterCharacter>(Pawn))
				{
					Character->bInputLocked = true;
				}
			}
		}
	}
	else
	{
		// 赛前倒计时结束：显示 GO! 并在 1 秒后隐藏；解锁本地输入
		ShowGoAndHide();

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (AShooterCharacter* Character = Cast<AShooterCharacter>(Pawn))
				{
					Character->bInputLocked = false;
				}
			}
		}
	}
}

void ACountdown::UpdateTimerDisplay(int32 Time)
{
	if (Time > 0)
	{
		CountdownText->SetText(FText::AsNumber(FMath::Max(Time, 0)));
		CountdownText->SetVisibility(true);
	}
	else
	{
		CountdownText->SetVisibility(false);
	}
}

void ACountdown::ShowGoAndHide()
{
	CountdownText->SetText(FText::FromString(TEXT("GO!")));
	CountdownText->SetVisibility(true);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HideGoTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(HideGoTimerHandle, this, &ACountdown::HideCountdownText, 1.0f, false);
	}
}

void ACountdown::HideCountdownText()
{
	CountdownText->SetVisibility(false);
}


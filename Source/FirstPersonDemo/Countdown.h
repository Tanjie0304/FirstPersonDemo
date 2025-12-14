// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Countdown.generated.h"

class UTextRenderComponent;
UCLASS()
class FIRSTPERSONDEMO_API ACountdown : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACountdown();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Countdown")
	UTextRenderComponent* CountdownText;

	FTimerHandle BindRetryTimerHandle;
	FTimerHandle HideGoTimerHandle;

	// °ó¶¨ÓÎÏ·×´Ì¬
	void TryBindToGameState();

	UFUNCTION()
	void OnPreGameCountdownUpdated(int32 NewTime);

	void UpdateTimerDisplay(int32 Time);
	void ShowGoAndHide();
	void HideCountdownText();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
};

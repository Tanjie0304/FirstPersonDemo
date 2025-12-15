// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetSpawner.generated.h"

class ATargetCube;
UCLASS()
class FIRSTPERSONDEMO_API ATargetSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATargetSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 可视化生成范围
	UPROPERTY(EditAnywhere, Category = "Spawner")
	class UBoxComponent* SpawnArea;

	// 生成的方块类
	UPROPERTY(EditAnywhere, Category = "Spawner")
	TSubclassOf<ATargetCube> TargetCubeClass;

	// 生成的数量
	UPROPERTY(EditAnywhere, Category = "Spawner")
	int32 SpawnCount = 10;

	// 每一波存活时间（秒），到时间后清理并重新生成
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float WaveInterval = 10.0f;

	// 当前这一波生成出来的方块
	UPROPERTY()
	TArray<ATargetCube*> SpawnedTargets;

	// 用于定时清理/重生的计时器
	FTimerHandle WaveTimerHandle;

	// 生成函数
	UFUNCTION()
	void SpawnTargets();

	// 定时回调：清除上一波 + 生成新一波
	UFUNCTION()
	void ClearAndRespawnTargets();
};

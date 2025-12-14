// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetSpawner.h"
#include "Components/BoxComponent.h"
#include "Variant_Shooter/ShooterGameState.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "TargetCube.h"

// Sets default values
ATargetSpawner::ATargetSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true; // 启用网络复制
	bAlwaysRelevant = true; // 始终相关，确保客户端也能看到生成的目标
	// 根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	// 创建一个盒子范围
	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATargetSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 只在服务器上生成和刷新
	if (HasAuthority())
	{
		// 先生成第一波
		SpawnTargets();

		// 设置一个循环计时器，每 WaveInterval 秒刷新一次
		if (WaveInterval > 0.f)
		{
			GetWorldTimerManager().SetTimer(
				WaveTimerHandle,
				this,
				&ATargetSpawner::ClearAndRespawnTargets,
				WaveInterval,
				true  // bLoop = true
			);
		}
	}
}


// Called every frame
void ATargetSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATargetSpawner::SpawnTargets()
{
	if (!TargetCubeClass) return;
	for (int32 i = 0; i < SpawnCount; i++)
	{
		// 在盒子范围内随机一个点
		FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(
			SpawnArea->GetComponentLocation(),
			SpawnArea->GetScaledBoxExtent()
		);
		RandomPoint.Z += 200.0f; // 提高Z轴位置，避免生成在地面以下
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this; // 设置拥有者

		ATargetCube* SpawnedCube = GetWorld()->SpawnActor<ATargetCube>(
			TargetCubeClass,
			RandomPoint,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (SpawnedCube)
		{
			// 记录生成的目标
			SpawnedTargets.Add(SpawnedCube);
			// UE_LOG(LogTemp, Log, TEXT("Spawned TargetCube %d with Score: %d"), i, SpawnedCube->Score);
		}
	}
}


void ATargetSpawner::ClearAndRespawnTargets()
{
	// 只允许服务器执行
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. 检查游戏是否已经结束（GameCountTime <= 0 就认为结束）
	AShooterGameState* GS = World->GetGameState<AShooterGameState>();
	if (GS && GS->GameCountTime <= 0)
	{
		// 游戏结束就停止计时器，不再生成新方块
		World->GetTimerManager().ClearTimer(WaveTimerHandle);
		return;
	}

	// 2. 先把上一波的方块全部 Destroy（没被打掉的）
	for (ATargetCube* Cube : SpawnedTargets)
	{
		if (IsValid(Cube))
		{
			Cube->Destroy();
		}
	}
	SpawnedTargets.Empty();

	// 3. 再生成新一波
	SpawnTargets();
}



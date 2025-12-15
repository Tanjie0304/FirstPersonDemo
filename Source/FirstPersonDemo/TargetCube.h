// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterCharacter.h"
#include "Variant_Shooter/ShooterGameMode.h"
#include "Components/TextRenderComponent.h"
#include "TargetCube.generated.h"


UCLASS()
class FIRSTPERSONDEMO_API ATargetCube : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATargetCube();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 立方体网格
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "target")
	UStaticMeshComponent* CubeMesh;

	// 分数属性
	UPROPERTY(ReplicatedUsing = OnRep_Score, VisibleAnywhere, BlueprintReadOnly, Category = "target")
	int32 Score = 0;

	// 分数显示文本组件
	UPROPERTY(VisibleAnywhere, Category = "target")
	UTextRenderComponent* ScoreText;

	// 分数为 1 时使用的材质
	UPROPERTY(EditAnywhere, Category = "target|Visual")
	UMaterialInterface* MaterialForScore1 = nullptr;

	// 分数为 2 时使用的材质
	UPROPERTY(EditAnywhere, Category = "target|Visual")
	UMaterialInterface* MaterialForScore2 = nullptr;


	// 被击中次数
	UPROPERTY(ReplicatedUsing = OnRep_HitCount, VisibleAnywhere, BlueprintReadOnly, Category = "target")
	int32 HitCount = 0;

	UFUNCTION()
	void OnRep_HitCount();

	// 缩放因子
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "target")
	float ScaleFactor = 2.0f; // 每次被击中时的缩放倍数

	// 被子弹击中事件
	UFUNCTION()
	void OnProjectileHit(AShooterCharacter* ShooterChar);

	UFUNCTION()
	void OnRep_Score();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	// 落到地面的碰撞事件
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 是否已经停止（防止重复触发 OnHit）
	bool bHasStopped = false;

public:
	void ApplyFirstHitScale();

	void UpdateMaterialByScore();
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HealthPickUp.generated.h"


class USphereComponent;
class UStaticMeshComponent;
class AShooterCharacter;

UCLASS()
class FIRSTPERSONDEMO_API AHealthPickUp : public AActor
{
	GENERATED_BODY()
    /** 碰撞球（用于检测角色进入范围） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USphereComponent* SphereCollision;

    /** 回血包主体的 mesh（小箱子之类） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* Mesh;

    /** 上方的红十字图标 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* IconMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USceneComponent* IconRoot;

public:	
	// Sets default values for this actor's properties
	AHealthPickUp();

protected:

    /** 如果 <= 0，则认为是回满血；否则为加固定血量 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health")
    float HealAmount = 0.0f;

    /** 拾取后多久在原地重新刷新 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup", meta = (ClampMin = 0, ClampMax = 120, Units="s"))
    float RespawnTime = 10.0f;

    /** Respawn 计时器 */
    FTimerHandle RespawnTimer;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 角色进入碰撞范围 */
    UFUNCTION()
    void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                   const FHitResult& SweepResult);

    /** 让回血包重新出现 */
    void RespawnPickup();

    /** 交给蓝图做刷新动画，结束时调用 FinishRespawn */
    UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta = (DisplayName = "OnRespawn"))
    void BP_OnRespawn();

    /** 刷新动画播放结束时调用，真正启用碰撞和 Tick */
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void FinishRespawn();
	
	
};

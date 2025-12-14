// Fill out your copyright notice in the Description page of Project Settings.


#include "Variant_Shooter/HealthPickUp.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Variant_Shooter/ShooterCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Sets default values
AHealthPickUp::AHealthPickUp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;          // 这个拾取物在网络中要复制
    bAlwaysRelevant = true;      // 所有客户端都能看到它的隐藏/显示变化

    // Root
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // 碰撞球
    SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
    SphereCollision->SetupAttachment(RootComponent);
    SphereCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 84.0f));
    SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SphereCollision->SetCollisionObjectType(ECC_WorldStatic);
    SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SphereCollision->bFillCollisionUnderneathForNavmesh = true;

    SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AHealthPickUp::OnOverlap);

    // 主体 mesh（一个小箱子、瓶子之类）
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SphereCollision);
    Mesh->SetCollisionProfileName(FName("NoCollision"));


    // 新增一个 IconRoot，当作图标的旋转根
    IconRoot = CreateDefaultSubobject<USceneComponent>(TEXT("IconRoot"));
    IconRoot->SetupAttachment(Mesh);

    // 图标 mesh（红十字），放在上面一点
    IconMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IconMesh"));
    IconMesh->SetupAttachment(IconRoot);
    IconMesh->SetCollisionProfileName(FName("NoCollision"));
    IconMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
}

// Called when the game starts or when spawned
void AHealthPickUp::BeginPlay()
{
	Super::BeginPlay();
	
}

void AHealthPickUp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // 清掉定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RespawnTimer);
    }
}

// Called every frame
void AHealthPickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (Mesh)
    {
        // 旋转功能（之前的旋转逻辑）
        FRotator NewRotation = Mesh->GetComponentRotation();
        NewRotation.Yaw += 20.0f * DeltaTime;  // 旋转速度
        Mesh->SetWorldRotation(NewRotation);

        // 浮动效果
        FVector CurrentLocation = Mesh->GetComponentLocation();

        // 浮动幅度和速度
        float FloatAmplitude = 1.0f;  // 浮动幅度（调整这个值来改变上下浮动的高度）
        float FloatSpeed = 2.0f;       // 浮动速度（调整这个值来改变浮动的快慢）

        // 计算新的 Z 轴位置，使用正弦波来实现上下浮动
        CurrentLocation.Z += FMath::Sin(GetWorld()->GetTimeSeconds() * FloatSpeed) * FloatAmplitude;

        // 设置新的位置
        Mesh->SetWorldLocation(CurrentLocation);
    }
}


void AHealthPickUp::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // 只在服务器处理回血 + 刷新逻辑
    if (!HasAuthority())
    {
        return;
    }

    AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(OtherActor);
    if (!ShooterChar)
    {
        return;
    }

    // 死了不回血
    if (!ShooterChar->IsAlive())
    {
        return;
    }

    // 血量已经满了就可以选择不处理（看你需求）
    // 这里直接判断：如果是加固定量，可以判断当前血是否满；如果是回满就无所谓
    if (HealAmount > 0.0f)
    {
        ShooterChar->Heal(HealAmount);
    }
    else
    {
        // HealAmount <= 0 表示回满血
        ShooterChar->HealToFull();
    }

    // 隐藏自己，关闭碰撞和 Tick
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);

    // 启动 Respawn 计时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RespawnTimer, this, &AHealthPickUp::RespawnPickup, RespawnTime, false);
    }
}

void AHealthPickUp::RespawnPickup()
{
    // 取消隐藏
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);

    // 交给蓝图做一个小动画（缩放/旋转之类），结束时在 BP 里调用 FinishRespawn
    BP_OnRespawn();
}

void AHealthPickUp::FinishRespawn()
{
    // 再次启用碰撞和 Tick（Tick 可要可不要）
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);
}

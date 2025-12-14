// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetCube.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ATargetCube::ATargetCube()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;          // Actor 本身要复制
	SetReplicateMovement(true); // Actor 的移动也要复制
	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	RootComponent = CubeMesh;

	// 让根组件的 Transform 也参与复制
	CubeMesh->SetIsReplicated(true);
	ScoreText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ScoreText"));
	ScoreText->SetupAttachment(RootComponent);
	ScoreText->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f)); // 文本在立方体上方显示
	ScoreText->SetHorizontalAlignment(EHTA_Center); // 水平居中
	ScoreText->SetTextRenderColor(FColor::Yellow);
}

// Called when the game starts or when spawned
void ATargetCube::BeginPlay()
{
	Super::BeginPlay();


	if (HasAuthority())
	{
		// 启用物理模拟
		CubeMesh->SetSimulatePhysics(true);
		// 启用重力
		CubeMesh->SetEnableGravity(true);
		// 设置随机分数
		Score = FMath::RandRange(1, 2);

		OnRep_Score();
	}

	//UE_LOG(LogTemp, Warning, TEXT("TargetCube BeginPlay: HasAuthority=%d, Sim=%d, Grav=%d, Mobility=%d"),
	//	HasAuthority(),
	//	CubeMesh->IsSimulatingPhysics() ? 1 : 0,
	//	CubeMesh->IsGravityEnabled() ? 1 : 0,
	//	(int32)CubeMesh->Mobility);

}

// Called every frame
void ATargetCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATargetCube::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 判断是否碰到地面
	if (!bHasStopped && OtherActor && OtherActor != this) 
	{
		CubeMesh->SetSimulatePhysics(false);  //  停止物理模拟
		CubeMesh->SetEnableGravity(false);    //  禁止再掉落
		CubeMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		CubeMesh->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

void ATargetCube::OnProjectileHit(AShooterCharacter* ShooterChar)
{
	// 仅在服务器处理击中逻辑
	if (!HasAuthority())
	{
		return;
	}
	// 增加击中次数
	HitCount++;
	// 第一次击中时放大方块
	if (HitCount == 1)
	{	
		ApplyFirstHitScale();
	}
	else if (HitCount >= 2)
	{
		// 第二次及以上击中时销毁方块，并且通知射击角色得分
		if (ShooterChar)
		{
			uint8 TeamByte = ShooterChar->TeamByte;
			if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
			{
				GM->IncrementTeamScore(TeamByte, Score);
			}
		}
		Destroy();
	}
}

void ATargetCube::ApplyFirstHitScale()
{
	FVector OldScale = CubeMesh->GetComponentScale();
	FVector NewScale = OldScale * ScaleFactor;
	// 获取方块的包围盒高度
	FVector BoxExtent = CubeMesh->Bounds.BoxExtent;
	float OldHeight = BoxExtent.Z * OldScale.Z * 2.0f; // 包围盒高度是半高的两倍
	float NewHeight = BoxExtent.Z * NewScale.Z * 2.0f;
	float DeltaHeight = NewHeight - OldHeight;
	// 调整方块位置，使其底部保持在原地面高度
	FVector OldLocation = GetActorLocation();
	FVector NewLocation = OldLocation + FVector(0.0f, 0.0f, DeltaHeight / 2.0f);
	CubeMesh->SetWorldScale3D(NewScale);
}

void ATargetCube::OnRep_Score()
{
	// 在客户端更新 UI 文本
	if (ScoreText)
	{
		ScoreText->SetText(FText::AsNumber(Score));
	}

	// 根据分数更换材质
	UpdateMaterialByScore();
}

void ATargetCube::OnRep_HitCount()
{
	// 只在第一次命中时做放大，避免第二次命中时又放大一遍
	if (HitCount == 1)
	{
		ApplyFirstHitScale();
	}
}

void ATargetCube::UpdateMaterialByScore()
{
	if (!CubeMesh)
	{
		return;
	}

	UMaterialInterface* UseMat = nullptr;

	switch (Score)
	{
	case 1:
		UseMat = MaterialForScore1;
		break;
	case 2:
		UseMat = MaterialForScore2;
		break;
	default:
		// 其他分数用默认材质，或者你以后扩展再往这加
		break;
	}

	if (UseMat)
	{
		CubeMesh->SetMaterial(0, UseMat);
	}
}


void ATargetCube::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATargetCube, Score);
	DOREPLIFETIME(ATargetCube, HitCount);
}


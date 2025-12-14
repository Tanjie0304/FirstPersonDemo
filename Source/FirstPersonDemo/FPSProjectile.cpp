// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSProjectile.h"


// Sets default values
AFPSProjectile::AFPSProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	if (!RootComponent) {
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	}

	if (!CollisionComponent)
	{
		// 用球体进行简单的碰撞展示
		CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
		// 设置球体的碰撞半径
		CollisionComponent->InitSphereRadius(15.0f);
		// 将球体的碰撞配置文件名称设置为"Projectile"
		CollisionComponent->BodyInstance.SetCollisionProfileName(TEXT("Projectile"));
		// 组件发生碰撞时调用OnHit函数
		CollisionComponent->OnComponentHit.AddDynamic(this, &AFPSProjectile::OnHit);
		// 将根组件设置为球体碰撞组件
		RootComponent = CollisionComponent;
	}

	if (!ProjectileMovementComponent)
	{
		// 使用此组件驱动发射物的移动
		ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
		ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
		ProjectileMovementComponent->InitialSpeed = 3000.0f;
		ProjectileMovementComponent->MaxSpeed = 3000.0f;
		ProjectileMovementComponent->bRotationFollowsVelocity = true; // 旋转跟随速度方向
		ProjectileMovementComponent->bShouldBounce = true; // 允许弹跳
		ProjectileMovementComponent->Bounciness = 0.3f; // 弹跳系数
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f; // 不受重力影响
	}

	if (!ProjectileMeshComponent)
	{
		ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMeshComponent"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh>Mesh(TEXT("'/Game/Weapons/GrenadeLauncher/Meshes/FirstPersonProjectileMesh.FirstPersonProjectileMesh'"));
		if (Mesh.Succeeded())
		{
			ProjectileMeshComponent->SetStaticMesh(Mesh.Object);
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterial>Material(TEXT("'/Game/Weapons/GrenadeLauncher/Materials/M_GrenadeLauncher.M_GrenadeLauncher'"));
	if (Material.Succeeded())
	{
		ProjectileMaterialInstance = UMaterialInstanceDynamic::Create(Material.Object, ProjectileMeshComponent);
		ProjectileMeshComponent->SetMaterial(0, ProjectileMaterialInstance);
		ProjectileMeshComponent->SetRelativeScale3D(FVector(0.09f, 0.09f, 0.09f));
		ProjectileMeshComponent->SetupAttachment(RootComponent);
	}
	InitialLifeSpan = 3.0f; // 设置发射物的生命周期为3秒
}

// Called when the game starts or when spawned
void AFPSProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFPSProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFPSProjectile::FireInDirection(const FVector& ShootDirection)
{
	ProjectileMovementComponent->Velocity = ShootDirection * ProjectileMovementComponent->InitialSpeed;
}

// 当发射物击中物体时会调用此函数
void AFPSProjectile::OnHit(UPrimitiveComponent* HitCompoent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	// 如果发射物击中了其他有效的物体
	if (OtherActor != this && OtherComponent->IsSimulatingPhysics())
	{
		// 可以在这里添加击中效果，例如播放声音、生成粒子效果等
		OtherComponent->AddImpulseAtLocation(ProjectileMovementComponent->Velocity * 10.0f, Hit.ImpactPoint); // 施加冲击力
	}
	// 销毁发射物
	Destroy();
}
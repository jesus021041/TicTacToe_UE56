// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseUnit.h"
#include "Math/UnrealMathUtility.h" // Necessario per le funzioni matematiche come il Random

// Sets default values
ABaseUnit::ABaseUnit()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Creazione del componente visivo (Mesh) e impostazione come radice (Root) dell'Actor
	UnitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
	SetRootComponent(UnitMesh);

	// Valori di default "sicuri" (questi verranno poi sovrascritti dalle classi figlie Sniper e Brawler)
	MovementRange = 0;
	AttackType = EAttackType::Melee;
	AttackRange = 0;
	MinDamage = 0;
	MaxDamage = 0;
	HealthPoints = 1;
	MaxHealthPoints = 1;
	PlayerOwner = -1;
	bHasActedThisTurn = false;
}

// Called when the game starts or when spawned
void ABaseUnit::BeginPlay()
{
	Super::BeginPlay();

	// Memorizziamo la vita massima all'inizio della partita per poterla usare nella UI (widget)
	MaxHealthPoints = HealthPoints;
}

// Called every frame
void ABaseUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

//LOGICA DELL'UNITA'

int32 ABaseUnit::CalculateDamage() const
{
	return FMath::RandRange(MinDamage, MaxDamage);
}

void ABaseUnit::TakeDamageAmount(int32 DamageAmount)
{
	// Sottraiamo il danno ai punti vita attuali
	HealthPoints -= DamageAmount;

	// Evitiamo che la vita vada sotto lo zero
	if (HealthPoints <= 0)
	{
		HealthPoints = 0;
		Die();
	}
}

void ABaseUnit::Die()
{
	
	Destroy();
}
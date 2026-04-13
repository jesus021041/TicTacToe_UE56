// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseUnit.h"
#include "Math/UnrealMathUtility.h" 
#include "TTT_GameMode.h"
#include "GameField.h"
#include "Tile.h"

// Sets default values
ABaseUnit::ABaseUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	UnitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
	SetRootComponent(UnitMesh);

	MovementRange = 0;
	AttackType = EAttackType::Melee;
	AttackRange = 0;
	MinDamage = 0;
	MaxDamage = 0;
	HealthPoints = 1;
	MaxHP = 1;
	PlayerOwner = -1;
	bHasActedThisTurn = false;
	bIsMoving = false;
}

// Called when the game starts or when spawned
void ABaseUnit::BeginPlay()
{
	Super::BeginPlay();

	MaxHP = HealthPoints;
}

// Called every frame
void ABaseUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//movimento -> Camminata
	if (bIsMoving && PathQueue.Num() > 0)
	{
		FVector TargetLoc = PathQueue[0];
		FVector CurrentLoc = GetActorLocation();

		// Se siamo vicini alla destinazione, agganciamoci e passiamo al prossimo punto
		if (FVector::Dist(CurrentLoc, TargetLoc) < 5.0f)
		{
			SetActorLocation(TargetLoc);
			PathQueue.RemoveAt(0);

			if (PathQueue.Num() == 0)
			{
				bIsMoving = false; // Percorso finito
			}
		}
		else
		{
			FVector NewLoc = FMath::VInterpConstantTo(CurrentLoc, TargetLoc, DeltaTime, 600.0f);
			SetActorLocation(NewLoc);
		}
	}
}

// CONVERTE LE COORDINATE LOGICHE IN SPAZIALI
void ABaseUnit::MoveAlongPath(const TArray<FVector2D>& PathCoords)
{
	if (PathCoords.Num() == 0) return;

	PathQueue.Empty();
	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());

	if (GameMode && GameMode->GField)
	{
		for (FVector2D Coord : PathCoords)
		{
			for (ATile* Tile : GameMode->GField->TileArray)
			{
				if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(Coord.X) &&
					FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(Coord.Y))
				{
					FVector WorldPos = Tile->GetActorLocation();
					WorldPos.Z += 60.0f;
					PathQueue.Add(WorldPos);
					break;
				}
			}
		}

		//ON -> l'animazione nel Tick
		bIsMoving = true;
	}
}

// LOGICA DELL'UNITA'

bool ABaseUnit::CanAttackTarget(ABaseUnit* TargetUnit) const
{
	if (!TargetUnit) return false;

	// Preleviamo l'altezza Z (assoluta nel mondo) di attaccante e bersaglio
	float MyZ = this->GetActorLocation().Z;
	float TargetZ = TargetUnit->GetActorLocation().Z;

	UE_LOG(LogTemp, Warning, TEXT("[Combat Check] La mia altezza: %f, Altezza Bersaglio: %f"), MyZ, TargetZ);

	//Margine di errore (10.0f)
	//Se il bersaglio è più in alto di noi, neghiamo l'attacco.
	if (TargetZ > MyZ + 10.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[Combat Check] BERSAGLIO TROPPO IN ALTO! Attacco negato per regola Cicala."));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Combat Check] Elevazione valida. L'attacco puo' proseguire."));
	return true; // Se e' allo stesso livello o piu' in basso, l'attacco e' valido!
}

int32 ABaseUnit::CalculateDamage() const
{
	return FMath::RandRange(MinDamage, MaxDamage);
}

void ABaseUnit::TakeDamageAmount(int32 DamageAmount)
{
	HealthPoints -= DamageAmount;

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
// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile.h"

// Sets default values
ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	SetRootComponent(Scene);
	StaticMeshComponent->SetupAttachment(Scene);

	Status = ETileStatus::EMPTY;
	PlayerOwner = -1;
	TileGridPosition = FVector2D(0, 0);

	// Default initialization
	ElevationLevel = 1;
	bIsObstacle = false;
}

void ATile::SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus)
{
	PlayerOwner = TileOwner;
	Status = TileStatus;
}

ETileStatus ATile::GetTileStatus()
{
	return Status;
}

int32 ATile::GetOwner()
{
	return PlayerOwner;
}

void ATile::SetGridPosition(const double InX, const double InY)
{
	TileGridPosition.Set(InX, InY);
}

FVector2D ATile::GetGridPosition()
{
	return TileGridPosition;
}

void ATile::SetElevationLevel(int32 Level)
{
	ElevationLevel = Level;

	// Il Livello 0 rappresenta l'acqua ed è un ostacolo non calpestabile
	if (ElevationLevel == 0)
	{
		bIsObstacle = true;
	}
	else
	{
		bIsObstacle = false;
	}

	// Cambia il materiale solo se l'array è stato popolato nell'editor di Unreal
	if (ElevationMaterials.IsValidIndex(ElevationLevel) && ElevationMaterials[ElevationLevel] != nullptr)
	{
		StaticMeshComponent->SetMaterial(0, ElevationMaterials[ElevationLevel]);
	}
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
	Super::BeginPlay();
}
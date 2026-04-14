// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"

// Sets default values
ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(Scene);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(Scene);

	// Inizializza la mesh per l'highlight
	HighlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightMesh"));
	HighlightMesh->SetupAttachment(Scene);

	// Di default, l'highlight è spento
	HighlightMesh->SetVisibility(false);

	//un cubo base arriva fino a Z=50. 
	// Mettiamo il piano a Z=52.0f x evitare compenetrazioni visive.
	HighlightMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 52.0f));

	//Riduciamo la scala dell'Highlight
	// Scaliamo il piano all'80%. In questo modo creerà un "bordo" del colore
	HighlightMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f));

	Status = ETileStatus::EMPTY;
	PlayerOwner = -1;
	TileGridPosition = FVector2D(0, 0);

	// Default initialization
	ElevationLevel = 1;
	bIsObstacle = false;
}

// IMPLEMENTAZIONE HIGHLIGHT
void ATile::SetTileHighlight(bool bHighlight, FLinearColor Color)
{
	if (HighlightMesh)
	{
		HighlightMesh->SetVisibility(bHighlight);

		if (bHighlight)
		{
			// Controllo Memory Leak: Se abbiamo già un'istanza dinamica
			UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(HighlightMesh->GetMaterial(0));

			if (!DynMaterial)
			{
				DynMaterial = HighlightMesh->CreateAndSetMaterialInstanceDynamic(0);
			}

			if (DynMaterial)
			{
				// Inietta il colore
				DynMaterial->SetVectorParameterValue(FName("Color"), Color);
			}
		}
	}
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
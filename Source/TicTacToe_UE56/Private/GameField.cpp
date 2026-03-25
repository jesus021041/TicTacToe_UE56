// Fill out your copyright notice in the Description page of Project Settings.

#include "GameField.h"
#include "Kismet/GameplayStatics.h"
#include "BaseSign.h"
#include "TTT_GameMode.h"
#include "Math/UnrealMathUtility.h" // Necessario per la funzione FMath::PerlinNoise2D

// Sets default values
AGameField::AGameField()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = false;

	// Parametri di default per il Perlin Noise
	NoiseScale = 0.15f;
	ZMultiplier = 50.0f; // Salto in altezza visivo per ogni Step Z
}

void AGameField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Utilizza il GridData esistente per impostare la Size (25x25)
	if (GridData)
	{
		Size = GridData->GridSize;
		WinSize = Size;
		TileSize = GridData->TileSize;
		CellPadding = GridData->CellPadding;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GridData has not been assigned."));
		return;
	}
	// Calcolo del moltiplicatore di posizione
	NextCellPositionMultiplier = (TileSize + TileSize * CellPadding) / TileSize;
}

// Called when the game starts or when spawned
void AGameField::BeginPlay()
{
	Super::BeginPlay();

	// Generazione di un Seed Casuale differente ad ogni partita (Requisito)
	RandomSeed = FMath::Rand();

	GenerateField();
	SpawnTowers();
}

void AGameField::ResetField()
{
	if (GetWorld())
	{
		for (ATile* Obj : TileArray)
		{
			// NOTA: In futuro il reset dovrà anche rimuovere le unità o rigenerare la mappa
			Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::EMPTY);
		}

		// Notifica gli oggetti registrati del reset
		OnResetEvent.Broadcast();

		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			GameMode->IsGameOver = false;
			GameMode->MoveCounter = 0;
		}
	}
}

void AGameField::GenerateField()
{
	// Offset basato sul seed casuale per variare la mappa
	FMath::RandInit(RandomSeed);
	float OffsetX = FMath::RandRange(-10000.0f, 10000.0f);
	float OffsetY = FMath::RandRange(-10000.0f, 10000.0f);

	for (int32 IndexX = 0; IndexX < Size; IndexX++)
	{
		for (int32 IndexY = 0; IndexY < Size; IndexY++)
		{
			// Calcolo Perlin Noise per determinare l'altezza
			float SampleX = (IndexX * NoiseScale) + OffsetX;
			float SampleY = (IndexY * NoiseScale) + OffsetY;
			float RawNoise = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));

			// Normalizzazione valore noise tra 0.0 e 1.0
			float NormalizedNoise = (RawNoise + 1.0f) / 2.0f;

			// Quantizzazione in 5 livelli (0, 1, 2, 3, 4)
			int32 ElevationLevel = FMath::Clamp(FMath::FloorToInt(NormalizedNoise * 5.0f), 0, 4);

			// Calcolo posizione spaziale
			FVector Location = GetRelativeLocationByXYPosition(IndexX, IndexY);
			Location.Z = ElevationLevel * ZMultiplier;

			// Spawn dell'attore Tile
			ATile* Obj = GetWorld()->SpawnActor<ATile>(TileClass, Location, FRotator::ZeroRotator);

			if (Obj)
			{
				const float TileScale = TileSize / 100.f;
				// Scaling Z leggermente variabile per dare spessore ai blocchi alti
				const float Zscaling = 0.2f + (ElevationLevel * 0.1f);
				Obj->SetActorScale3D(FVector(TileScale, TileScale, Zscaling));

				Obj->SetGridPosition(IndexX, IndexY);

				// CORREZIONE ERRORE: Il nome corretto della funzione definita in Tile.h è SetElevationLevel
				Obj->SetElevationLevel(ElevationLevel);

				TileArray.Add(Obj);
				TileMap.Add(FVector2D(IndexX, IndexY), Obj);
			}
		}
	}
}

void AGameField::SpawnTowers()
{
	// Coordinate ideali per il piazzamento simmetrico (Requisito)
	TArray<FVector2D> IdealTowerPositions = {
		FVector2D(12, 12), // Torre Centrale
		FVector2D(5, 12),  // Torre Laterale Sinistra
		FVector2D(19, 12)  // Torre Laterale Destra
	};

	for (FVector2D TargetPos : IdealTowerPositions)
	{
		ATile* BestTile = GetNearestValidTileForTower(TargetPos);
		if (BestTile)
		{
			// Segna la cella come occupata da una torre (non calpestabile)
			// Nota: aggiungeremo la logica bIsTower in ATile se necessario
			UE_LOG(LogTemp, Log, TEXT("Tower spawned at X:%f Y:%f"), BestTile->GetGridPosition().X, BestTile->GetGridPosition().Y);
		}
	}
}

ATile* AGameField::GetNearestValidTileForTower(FVector2D TargetPos)
{
	ATile* BestTile = nullptr;
	float MinDist = MAX_FLT;

	for (ATile* Tile : TileArray)
	{
		// Criteri: il tile più vicino che non sia acqua (Elevation > 0)
		// Accediamo alla proprietà ElevationLevel tramite Tile
		if (Tile->ElevationLevel > 0)
		{
			float Dist = FVector2D::Distance(TargetPos, Tile->GetGridPosition());
			if (Dist < MinDist)
			{
				MinDist = Dist;
				BestTile = Tile;
			}
		}
	}
	return BestTile;
}

FVector2D AGameField::GetPosition(const FHitResult& Hit)
{
	if (ATile* Tile = Cast<ATile>(Hit.GetActor()))
	{
		return Tile->GetGridPosition();
	}
	return FVector2D(-1, -1);
}

TArray<ATile*>& AGameField::GetTileArray()
{
	return TileArray;
}

FVector AGameField::GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const
{
	return TileSize * NextCellPositionMultiplier * FVector(InX, InY, 0);
}

FVector2D AGameField::GetXYPositionByRelativeLocation(const FVector& Location) const
{
	const double XPos = Location.X / (TileSize * NextCellPositionMultiplier);
	const double YPos = Location.Y / (TileSize * NextCellPositionMultiplier);
	return FVector2D(FMath::RoundToDouble(XPos), FMath::RoundToDouble(YPos));
}
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

	// Scala per avere macchie ampie e naturali.
	NoiseScale = 0.08f;
	ZMultiplier = 60.0f; // Leggermente aumentato per rendere i gradini più distinguibili
}

void AGameField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Utilizza il GridData esistente per impostare la Size (Requisito: 25x25)
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

	NextCellPositionMultiplier = (TileSize + TileSize * CellPadding) / TileSize;
}

// Called when the game starts or when spawned
void AGameField::BeginPlay()
{
	Super::BeginPlay();

	// Generazione di un Seed Casuale differente ad ogni esecuzione
	RandomSeed = FMath::Rand();

	GenerateField();

	// Piazzamento delle 3 Torri (Obiettivi)
	SpawnTowers();
}

void AGameField::ResetField()
{
	if (GetWorld())
	{
		for (ATile* Obj : TileArray)
		{
			Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::EMPTY);
		}

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
	FMath::RandInit(RandomSeed);

	int32 TotalCells = Size * Size; // 625 per una mappa 25x25

	// QUOTE ESATTE (Garantiscono colori bilanciati ad ogni partita)
	int32 CountBlue = FMath::RoundToInt(TotalCells * 0.12f);   // ~12% Livello 0 (Acqua)
	int32 CountRed = FMath::RoundToInt(TotalCells * 0.18f);    // ~18% Livello 4 (Rosso)
	int32 CountGreen = FMath::RoundToInt(TotalCells * 0.25f);  // ~25% Livello 1 (Verde)
	int32 CountYellow = FMath::RoundToInt(TotalCells * 0.25f); // ~25% Livello 2 (Giallo)
	// Il restante ~20% sarà Livello 3 (Arancio)

	// FUNZIONE DI VERIFICA CONNESSIONE (Evita isole irraggiungibili)
	auto CheckConnectivity = [&](const TArray<int32>& Elevations) -> bool {
		TArray<bool> Visited;
		Visited.Init(false, TotalCells);
		int32 StartIndex = -1;

		// Trova la prima cella di terra (non acqua)
		for (int32 i = 0; i < TotalCells; i++) {
			if (Elevations[i] > 0) {
				StartIndex = i;
				break;
			}
		}
		if (StartIndex == -1) return false;

		TArray<int32> Queue;
		Queue.Add(StartIndex);
		Visited[StartIndex] = true;
		int32 VisitedCount = 0;

		// Flood-Fill per scorrere tutta la terra accessibile
		while (Queue.Num() > 0) {
			int32 Curr = Queue[0];
			Queue.RemoveAt(0);
			VisitedCount++;

			int32 CX = Curr % Size;
			int32 CY = Curr / Size;

			// Su, Giù, Destra, Sinistra
			int32 OffsetsX[] = { 0, 0, 1, -1 };
			int32 OffsetsY[] = { 1, -1, 0, 0 };

			for (int32 n = 0; n < 4; n++) {
				int32 NX = CX + OffsetsX[n];
				int32 NY = CY + OffsetsY[n];
				if (NX >= 0 && NX < Size && NY >= 0 && NY < Size) {
					int32 NIndex = NY * Size + NX;
					// Se non è visitato e NON è acqua (>0)
					if (!Visited[NIndex] && Elevations[NIndex] > 0) {
						Visited[NIndex] = true;
						Queue.Add(NIndex);
					}
				}
			}
		}
		// La mappa è valida se le celle di terra visitate corrispondono al totale della terra
		return VisitedCount == (TotalCells - CountBlue);
		};

	bool bMapValid = false;
	TArray<int32> FinalElevations;
	FinalElevations.Init(0, TotalCells);

	int32 MaxAttempts = 1000;
	int32 Attempts = 0;

	// LOOP DI GENERAZIONE: Riprova finché non trova una mappa senza isole
	while (!bMapValid && Attempts < MaxAttempts)
	{
		Attempts++;
		float OffsetX = FMath::RandRange(-50000.0f, 50000.0f);
		float OffsetY = FMath::RandRange(-50000.0f, 50000.0f);

		struct FNoiseCell { int32 Index; float NoiseVal; };
		TArray<FNoiseCell> NoiseCells;

		for (int32 IndexX = 0; IndexX < Size; IndexX++) {
			for (int32 IndexY = 0; IndexY < Size; IndexY++) {
				float SampleX = (IndexX * NoiseScale) + OffsetX;
				float SampleY = (IndexY * NoiseScale) + OffsetY;

				// Rileviamo la topografia base
				float Noise = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
				NoiseCells.Add({ IndexY * Size + IndexX, Noise });
			}
		}

		// Ordiniamo le celle per valore (da valle a picco)
		NoiseCells.Sort([](const FNoiseCell& A, const FNoiseCell& B) {
			return A.NoiseVal < B.NoiseVal;
			});

		// Mappatura Esatta dei Colori (Garantisce le percentuali desiderate)
		for (int32 i = 0; i < TotalCells; i++) {
			int32 Level = 0;
			if (i < CountBlue) Level = 0;
			else if (i < CountBlue + CountGreen) Level = 1;
			else if (i < CountBlue + CountGreen + CountYellow) Level = 2;
			else if (i < TotalCells - CountRed) Level = 3;
			else Level = 4;

			FinalElevations[NoiseCells[i].Index] = Level;
		}

		// Controlla se ci sono zone isolate
		bMapValid = CheckConnectivity(FinalElevations);
	}

	if (!bMapValid) {
		UE_LOG(LogTemp, Warning, TEXT("Impossibile trovare mappa senza isole. Forzata ultima generata."));
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Mappa perfetta generata in %d tentativi"), Attempts);
	}

	// ----------------------------------------------------------------------
	// SPAWN FISICO DEI TILE
	// ----------------------------------------------------------------------
	for (int32 IndexX = 0; IndexX < Size; IndexX++)
	{
		for (int32 IndexY = 0; IndexY < Size; IndexY++)
		{
			int32 ElevationLevel = FinalElevations[IndexY * Size + IndexX];

			FVector Location = GetRelativeLocationByXYPosition(IndexX, IndexY);
			Location.Z = ElevationLevel * ZMultiplier;

			ATile* Obj = GetWorld()->SpawnActor<ATile>(TileClass, Location, FRotator::ZeroRotator);

			if (Obj)
			{
				const float TileScale = TileSize / 100.f;
				const float Zscaling = 0.2f + (ElevationLevel * 0.05f);
				Obj->SetActorScale3D(FVector(TileScale, TileScale, Zscaling));

				Obj->SetGridPosition(IndexX, IndexY);
				Obj->SetElevationLevel(ElevationLevel);

				TileArray.Add(Obj);
				TileMap.Add(FVector2D(IndexX, IndexY), Obj);
			}
		}
	}
}

void AGameField::SpawnTowers()
{
	// Coordinate ideali da PDF
	TArray<FVector2D> IdealTowerPositions = {
		FVector2D(12, 12), // Centro
		FVector2D(5, 12),  // Sinistra
		FVector2D(12, 19)  // Destra
	};

	for (FVector2D TargetPos : IdealTowerPositions)
	{
		ATile* BestTile = GetNearestValidTileForTower(TargetPos);

		if (BestTile)
		{
			// Log per debug nel terminale di Unreal
			UE_LOG(LogTemp, Log, TEXT("Tower Objective placed at X:%f Y:%f"), BestTile->GetGridPosition().X, BestTile->GetGridPosition().Y);
		}
	}
}

ATile* AGameField::GetNearestValidTileForTower(FVector2D TargetPos)
{
	ATile* BestTile = nullptr;
	float MinDist = MAX_FLT;

	for (ATile* Tile : TileArray)
	{
		// Le torri non possono stare nell'acqua (Livello 0)
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
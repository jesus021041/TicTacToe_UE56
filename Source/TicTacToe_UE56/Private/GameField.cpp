// Fill out your copyright notice in the Description page of Project Settings.

#include "GameField.h"
#include "Kismet/GameplayStatics.h"
#include "BaseSign.h"
#include "TTT_GameMode.h"
#include "Math/UnrealMathUtility.h" // Necessario per la funzione FMath::PerlinNoise2D
#include "Misc/DateTime.h"          // Necessario per il vero random basato sul tempo
#include "Math/RandomStream.h"      // FIX: Gestione corretta e indipendente del Seed

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
		RandomSeed = GridData->Seed;
		NoiseScale = GridData->N_Scale;
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

	// FIX SEED BUG: In Unreal PIE, il random globale può dare la stessa sequenza.
	// Usiamo i Ticks del tempo reale di sistema per un vero Random!
	RandomSeed = FDateTime::Now().GetTicks() % 9999999;

	UE_LOG(LogTemp, Warning, TEXT("======================================="));
	UE_LOG(LogTemp, Warning, TEXT("=== INIZIO GENERAZIONE MAPPA 25x25 ==="));
	UE_LOG(LogTemp, Warning, TEXT("Seed Casuale Generato dal Tempo: %d"), RandomSeed);
	UE_LOG(LogTemp, Warning, TEXT("======================================="));

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
	// FRandomStream garantisce che tutti i random (float e int) siano forzatamente derivati dal nostro Seed
	FRandomStream RandomStream(RandomSeed);

	int32 TotalCells = Size * Size; // 625 per una mappa 25x25

	// QUOTE ESATTE (Garantiscono colori bilanciati ad ogni partita come richiesto)
	int32 CountBlue = FMath::RoundToInt(TotalCells * 0.18f);   // ~18% Livello 0 (Acqua)
	int32 CountRed = FMath::RoundToInt(TotalCells * 0.12f);    // ~12% Livello 4 (Rosso)
	int32 CountGreen = FMath::RoundToInt(TotalCells * 0.25f);  // ~25% Livello 1 (Verde)
	int32 CountYellow = FMath::RoundToInt(TotalCells * 0.25f); // ~25% Livello 2 (Giallo)
	// Il restante ~20% sarà Livello 3 (Arancio)

	UE_LOG(LogTemp, Log, TEXT("Target Celle calcolato: Acqua:%d, Pianura:%d, Montagna Bassa:%d, Vetta:%d"), CountBlue, CountGreen, CountYellow, CountRed);

	TArray<int32> FinalElevations;
	FinalElevations.Init(0, TotalCells);

	// Utilizziamo il RandomStream per ottenere offset in virgola mobile sempre nuovi ad ogni avvio!
	float OffsetX = RandomStream.FRandRange(-50000.0f, 50000.0f);
	float OffsetY = RandomStream.FRandRange(-50000.0f, 50000.0f);

	struct FNoiseCell { int32 Index; float NoiseVal; };
	TArray<FNoiseCell> NoiseCells;
	TArray<float> RawNoises;
	RawNoises.Init(0.0f, TotalCells);

	// Calcolo del Perlin Noise per ogni cella
	for (int32 IndexX = 0; IndexX < Size; IndexX++) {
		for (int32 IndexY = 0; IndexY < Size; IndexY++) {
			float SampleX = (IndexX * NoiseScale) + OffsetX;
			float SampleY = (IndexY * NoiseScale) + OffsetY;

			float Noise = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
			int32 FlatIndex = IndexY * Size + IndexX;
			NoiseCells.Add({ FlatIndex, Noise });
			RawNoises[FlatIndex] = Noise; // Salviamo il rumore grezzo per la riparazione
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

	// =====================================================================
	// ALGORITMO DI RIPARAZIONE ISOLE (Garantisce 1 singolo continente)
	// =====================================================================
	TArray<int32> ComponentLabels;
	ComponentLabels.Init(-1, TotalCells);
	TMap<int32, int32> ComponentSizes;
	int32 CurrentLabel = 0;

	// 1. Identifica tutti i continenti (masse di terra collegate) usando il Flood Fill
	for (int32 i = 0; i < TotalCells; i++) {
		if (FinalElevations[i] > 0 && ComponentLabels[i] == -1) {
			int32 CompSize = 0;
			TArray<int32> Queue;
			Queue.Add(i);
			ComponentLabels[i] = CurrentLabel;

			while (Queue.Num() > 0) {
				int32 Curr = Queue[0];
				Queue.RemoveAt(0);
				CompSize++;

				int32 CX = Curr % Size;
				int32 CY = Curr / Size;

				int32 OffsetsX[] = { 0, 0, 1, -1 };
				int32 OffsetsY[] = { 1, -1, 0, 0 };

				for (int32 n = 0; n < 4; n++) {
					int32 NX = CX + OffsetsX[n];
					int32 NY = CY + OffsetsY[n];
					if (NX >= 0 && NX < Size && NY >= 0 && NY < Size) {
						int32 NIndex = NY * Size + NX;
						if (FinalElevations[NIndex] > 0 && ComponentLabels[NIndex] == -1) {
							ComponentLabels[NIndex] = CurrentLabel;
							Queue.Add(NIndex);
						}
					}
				}
			}
			ComponentSizes.Add(CurrentLabel, CompSize);
			CurrentLabel++;
		}
	}

	// 2. Trova il continente principale (quello con più celle)
	int32 MainComponentLabel = -1;
	int32 MaxSize = 0;
	for (auto& Pair : ComponentSizes) {
		if (Pair.Value > MaxSize) {
			MaxSize = Pair.Value;
			MainComponentLabel = Pair.Key;
		}
	}

	// 3. Affonda le isole minori (le fa diventare acqua)
	int32 IsolatedLandCount = 0;
	for (int32 i = 0; i < TotalCells; i++) {
		if (FinalElevations[i] > 0 && ComponentLabels[i] != MainComponentLabel) {
			FinalElevations[i] = 0;
			IsolatedLandCount++;
		}
	}

	// 4. Per mantenere le percentuali perfette, trasforma l'acqua costiera in pianura
	if (IsolatedLandCount > 0) {
		UE_LOG(LogTemp, Warning, TEXT("Trovate %d celle isolate. Avvio riparazione mappa (creazione ponti naturali)..."), IsolatedLandCount);

		for (int32 k = 0; k < IsolatedLandCount; k++) {
			int32 BestWaterIndex = -1;
			float BestNoise = -MAX_FLT;

			// Cerca la cella d'acqua migliore adiacente al continente
			for (int32 i = 0; i < TotalCells; i++) {
				if (FinalElevations[i] == 0) {
					int32 CX = i % Size;
					int32 CY = i / Size;
					bool bAdjacentToLand = false;

					int32 OffsetsX[] = { 0, 0, 1, -1 };
					int32 OffsetsY[] = { 1, -1, 0, 0 };

					for (int32 n = 0; n < 4; n++) {
						int32 NX = CX + OffsetsX[n];
						int32 NY = CY + OffsetsY[n];
						if (NX >= 0 && NX < Size && NY >= 0 && NY < Size) {
							int32 NIndex = NY * Size + NX;
							// Deve toccare la terra
							if (FinalElevations[NIndex] > 0) {
								bAdjacentToLand = true;
								break;
							}
						}
					}

					if (bAdjacentToLand) {
						// Preferiamo l'acqua meno profonda (Noise più alto)
						if (RawNoises[i] > BestNoise) {
							BestNoise = RawNoises[i];
							BestWaterIndex = i;
						}
					}
				}
			}

			if (BestWaterIndex != -1) {
				FinalElevations[BestWaterIndex] = 1; // Nuova terra (Pianura verde)
				RawNoises[BestWaterIndex] = -MAX_FLT; // Evita di pescare la stessa cella se ci sono più isole da coprire
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Riparazione completata con successo! Mappa validata."));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Mappa perfetta! Nessuna isola isolata generata."));
	}
	// =====================================================================


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

	UE_LOG(LogTemp, Warning, TEXT("======================================="));
	UE_LOG(LogTemp, Warning, TEXT("=== RESOCONTO FINALE MAPPA GENERATA ==="));
	UE_LOG(LogTemp, Warning, TEXT("Celle Acqua (Livello 0 - Blu)     : %d"), CountBlue);
	UE_LOG(LogTemp, Warning, TEXT("Celle Pianura (Livello 1 - Verde) : %d"), CountGreen);
	UE_LOG(LogTemp, Warning, TEXT("Celle Colline (Livello 2 - Giallo): %d"), CountYellow);
	UE_LOG(LogTemp, Warning, TEXT("Celle Montagne (Livello 3 - Aran.): %d"), (TotalCells - CountBlue - CountGreen - CountYellow - CountRed));
	UE_LOG(LogTemp, Warning, TEXT("Celle Vette (Livello 4 - Rosso)   : %d"), CountRed);
	UE_LOG(LogTemp, Warning, TEXT("======================================="));
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
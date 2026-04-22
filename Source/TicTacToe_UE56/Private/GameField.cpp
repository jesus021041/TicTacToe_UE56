// Fill out your copyright notice in the Description page of Project Settings.

#include "GameField.h"
#include "Kismet/GameplayStatics.h"
#include "BaseSign.h"
#include "TTT_GameMode.h"
#include "Math/UnrealMathUtility.h" 
#include "Misc/DateTime.h"         
#include "Math/RandomStream.h"      

// Sets default values
AGameField::AGameField()
{
	PrimaryActorTick.bCanEverTick = false;

	// Scala per avere macchie ampie e naturali.
	NoiseScale = 0.08f;
	ZMultiplier = 60.0f;
}

void AGameField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	//Intro x parametri:
	//->GRANDEZZA DINAMICA
	Size = 0;

	// 1. Proviamo a leggere il valore scelto nel menu
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		UTTT_GameInstance* GI = Cast<UTTT_GameInstance>(GetWorld()->GetGameInstance());
		if (GI && GI->CustomGridSize > 0)
		{
			Size = GI->CustomGridSize;
		}
	}

	if (GridData)
	{
		// 2. Se non c'era nessun valore (o siamo fermi nell'editor) -> usare: default (25*25)
		if (Size <= 0)
		{
			Size = GridData->GridSize;
		}

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

void AGameField::BeginPlay()
{
	Super::BeginPlay();

	//SEED DINAMICO
	RandomSeed = 0;

	//I. Verifica se l'utente ha inserito un seed nel menu
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		UTTT_GameInstance* GI = Cast<UTTT_GameInstance>(GetWorld()->GetGameInstance());
		if (GI && GI->CustomSeed > 0)
		{
			RandomSeed = GI->CustomSeed;
		}
	}

	//II. Se l'utente ha lasciato 0 -> CASUALE
	if (RandomSeed <= 0)
	{
		RandomSeed = FDateTime::Now().GetTicks() % 9999999;
	}

	UE_LOG(LogTemp, Warning, TEXT("-----> INIZIO GENERAZIONE MAPPA %dx%d <-----"), Size, Size);
	UE_LOG(LogTemp, Warning, TEXT("Seed Casuale Generato dal Tempo: %d"), RandomSeed);
	UE_LOG(LogTemp, Warning, TEXT("----------------------------------------------"));

	GenerateField();
	SpawnTowers();
}

void AGameField::ResetField()
{
	if (GetWorld())
	{
		for (ATile* Obj : TileArray)
		{
			// PROTEZIONE ACQUA E TORRI: 
			//se è acqua OR una torre, resta occupata in modo permanente
			if (Obj->ElevationLevel == 0 || Obj->Tags.Contains(FName("TowerTile")))
			{
				Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::OCCUPIED);
			}
			else
			{
				Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::EMPTY);
			}
		}

		OnResetEvent.Broadcast();

		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			GameMode->IsGameOver = false;
			GameMode->MoveCounter = 0;
			GameMode->PerformCoinFlip();
		}
	}
}

void AGameField::GenerateField()
{
	FRandomStream RandomStream(RandomSeed);

	int32 TotalCells = Size * Size;

	int32 CountBlue = FMath::RoundToInt(TotalCells * 0.18f);
	int32 CountRed = FMath::RoundToInt(TotalCells * 0.12f);
	int32 CountGreen = FMath::RoundToInt(TotalCells * 0.25f);
	int32 CountYellow = FMath::RoundToInt(TotalCells * 0.25f);

	TArray<int32> FinalElevations;
	FinalElevations.Init(0, TotalCells);

	float OffsetX = RandomStream.FRandRange(-50000.0f, 50000.0f);
	float OffsetY = RandomStream.FRandRange(-50000.0f, 50000.0f);

	struct FNoiseCell { int32 Index; float NoiseVal; };
	TArray<FNoiseCell> NoiseCells;
	TArray<float> RawNoises;
	RawNoises.Init(0.0f, TotalCells);

	for (int32 IndexX = 0; IndexX < Size; IndexX++) {
		for (int32 IndexY = 0; IndexY < Size; IndexY++) {
			float SampleX = (IndexX * NoiseScale) + OffsetX;
			float SampleY = (IndexY * NoiseScale) + OffsetY;

			float Noise = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
			int32 FlatIndex = IndexY * Size + IndexX;
			NoiseCells.Add({ FlatIndex, Noise });
			RawNoises[FlatIndex] = Noise;
		}
	}

	NoiseCells.Sort([](const FNoiseCell& A, const FNoiseCell& B) {
		return A.NoiseVal < B.NoiseVal;
		});

	for (int32 i = 0; i < TotalCells; i++) {
		int32 Level = 0;
		if (i < CountBlue) Level = 0;
		else if (i < CountBlue + CountGreen) Level = 1;
		else if (i < CountBlue + CountGreen + CountYellow) Level = 2;
		else if (i < TotalCells - CountRed) Level = 3;
		else Level = 4;

		FinalElevations[NoiseCells[i].Index] = Level;
	}

	TArray<int32> ComponentLabels;
	ComponentLabels.Init(-1, TotalCells);
	TMap<int32, int32> ComponentSizes;
	int32 CurrentLabel = 0;

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

	int32 MainComponentLabel = -1;
	int32 MaxSize = 0;
	for (auto& Pair : ComponentSizes) {
		if (Pair.Value > MaxSize) {
			MaxSize = Pair.Value;
			MainComponentLabel = Pair.Key;
		}
	}

	int32 IsolatedLandCount = 0;
	for (int32 i = 0; i < TotalCells; i++) {
		if (FinalElevations[i] > 0 && ComponentLabels[i] != MainComponentLabel) {
			FinalElevations[i] = 0;
			IsolatedLandCount++;
		}
	}

	if (IsolatedLandCount > 0) {
		for (int32 k = 0; k < IsolatedLandCount; k++) {
			int32 BestWaterIndex = -1;
			float BestNoise = -MAX_FLT;

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
							if (FinalElevations[NIndex] > 0) {
								bAdjacentToLand = true;
								break;
							}
						}
					}

					if (bAdjacentToLand) {
						if (RawNoises[i] > BestNoise) {
							BestNoise = RawNoises[i];
							BestWaterIndex = i;
						}
					}
				}
			}

			if (BestWaterIndex != -1) {
				FinalElevations[BestWaterIndex] = 1;
				RawNoises[BestWaterIndex] = -MAX_FLT;
			}
		}
	}

	// SPAWN FISICO DEI TILE
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

				//BLUEPRINT DEI COLORI
				Obj->SetElevationLevel(ElevationLevel);

				if (ElevationLevel == 0)
				{
					Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::OCCUPIED);
				}
				else
				{
					Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::EMPTY);
				}

				TileArray.Add(Obj);
				TileMap.Add(FVector2D(IndexX, IndexY), Obj);
			}
		}
	}
}

void AGameField::SpawnTowers()
{
	//->Alg adattivo x piazzare
	// Si adatta automaticamente alla variabile "Size" della griglia.
	int32 CenterXY = Size / 2;
	int32 LeftX = Size / 4;
	int32 RightX = Size - (Size / 4);

	TArray<FVector2D> IdealTowerPositions = {
		FVector2D(CenterXY, CenterXY), // Centro esatto
		FVector2D(LeftX, CenterXY),    // Lato Sinistro
		FVector2D(RightX, CenterXY)    // Lato Destro
	};

	for (int32 i = 0; i < IdealTowerPositions.Num(); i++)
	{
		FVector2D TargetPos = IdealTowerPositions[i];
		ATile* BestTile = GetNearestValidTileForTower(TargetPos);

		if (BestTile)
		{
			//La cella contenente la Torre -> un Ostacolo Fisico.
			BestTile->SetTileStatus(NOT_ASSIGNED, ETileStatus::OCCUPIED);

			if (TowerClass != nullptr)
			{
				FVector SpawnLocation = BestTile->GetActorLocation();
				SpawnLocation.Z += 80.0f;
				GetWorld()->SpawnActor<AActor>(TowerClass, SpawnLocation, FRotator::ZeroRotator);
			}
			//Tag -> x trovare le torri ++ facile
			BestTile->Tags.Add(FName("TowerTile"));
		}
	}
}

ATile* AGameField::GetNearestValidTileForTower(FVector2D TargetPos)
{
	ATile* BestTile = nullptr;
	float MinDist = MAX_FLT;

	for (ATile* Tile : TileArray)
	{
		// Cerca la zona di terra (Elevation > 0)
		if (Tile->ElevationLevel > 0 && Tile->GetTileStatus() == ETileStatus::EMPTY)
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
	if (ATile* Tile = Cast<ATile>(Hit.GetActor())) return Tile->GetGridPosition();
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
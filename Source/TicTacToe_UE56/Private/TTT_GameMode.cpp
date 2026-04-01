// Fill out your copyright notice in the Description page of Project Settings.

#include "TTT_GameMode.h"
#include "TTT_PlayerController.h"
#include "TTT_HumanPlayer.h"
#include "TTT_RandomPlayer.h"
#include "Tile.h" 
#include "EngineUtils.h"

ATTT_GameMode::ATTT_GameMode()
{
	PlayerControllerClass = ATTT_PlayerController::StaticClass();
	DefaultPawnClass = ATTT_HumanPlayer::StaticClass();
}

void ATTT_GameMode::BeginPlay()
{
	Super::BeginPlay();

	IsGameOver = false;
	MoveCounter = 0;
	Player0UnitsPlaced = 0;
	Player1UnitsPlaced = 0;
	CurrentGameState = EGameState::CoinFlip;

	ATTT_HumanPlayer* HumanPlayer = GetWorld()->GetFirstPlayerController()->GetPawn<ATTT_HumanPlayer>();
	if (!IsValid(HumanPlayer)) return;

	if (GridData)
	{
		FieldSize = GridData->GridSize;
		TileSize = GridData->TileSize;
		CellPadding = GridData->CellPadding;
	}

	if (GameFieldClass != nullptr)
	{
		GField = GetWorld()->SpawnActor<AGameField>(GameFieldClass);
	}

	float CameraPosX = ((TileSize * FieldSize) + ((FieldSize - 1) * TileSize * CellPadding)) * 0.5f;
	FVector CameraPos(CameraPosX, CameraPosX, 4000.0f);
	HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());

	Players.Add(HumanPlayer);

	TSubclassOf<APawn> AIClassToSpawn = GetWorld()->GetGameInstance<UTTT_GameInstance>()->SelectedAIClass;
	auto* AI = GetWorld()->SpawnActor<ITTT_PlayerInterface>(AIClassToSpawn);
	Players.Add(AI);

	PerformCoinFlip();
}

void ATTT_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(ResetTimerHandle);
}

void ATTT_GameMode::PerformCoinFlip()
{
	for (int32 i = 0; i < Players.Num(); i++)
	{
		if (Players[i]) Players[i]->PlayerNumber = i;
	}

	StartingPlayer = FMath::RandRange(0, 1);
	CurrentPlayer = StartingPlayer;
	CurrentGameState = EGameState::Placement;
	Players[CurrentPlayer]->OnTurn();
}

void ATTT_GameMode::SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition)
{
	if (IsGameOver || PlayerNumber != CurrentPlayer) return;

	FVector2D GridPosRaw = GField->GetXYPositionByRelativeLocation(SpawnPosition);
	FVector2D GridPos(FMath::RoundToDouble(GridPosRaw.X), FMath::RoundToDouble(GridPosRaw.Y));

	if (CurrentGameState == EGameState::Placement)
	{
		int32& UnitsPlaced = (CurrentPlayer == 0) ? Player0UnitsPlaced : Player1UnitsPlaced;
		TSubclassOf<ABaseUnit> UnitToSpawn = (UnitsPlaced == 0) ? SniperClass : BrawlerClass;

		if (!UnitToSpawn || !GField)
		{
			Players[CurrentPlayer]->OnTurn();
			return;
		}

		ATile* TargetTile = nullptr;

		for (ATile* Tile : GField->TileArray)
		{
			if (Tile && FMath::IsNearlyEqual(Tile->GetGridPosition().X, GridPos.X) && FMath::IsNearlyEqual(Tile->GetGridPosition().Y, GridPos.Y))
			{
				TargetTile = Tile;
				break;
			}
		}

		// =========================================================================
		// FIX ACQUA ELEGANTE: Controlliamo la variabile ElevationLevel
		// Il livello 0 è l'acqua. Non calpestabile!
		// =========================================================================
		if (TargetTile && TargetTile->ElevationLevel == 0)
		{
			if (CurrentPlayer == 0) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Errore: L'Acqua non e' calpestabile!"));
			Players[CurrentPlayer]->OnTurn();
			return; // BLOCCA IL PIAZZAMENTO
		}

		// Controllo ostacoli fisici (Altre unità o Torri)
		if (!TargetTile || TargetTile->GetTileStatus() != ETileStatus::EMPTY)
		{
			if (CurrentPlayer == 0) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Errore: Cella occupata da una Torre o Unita'!"));
			Players[CurrentPlayer]->OnTurn();
			return;
		}

		// Controllo validità aree di schieramento
		if (CurrentPlayer == 0 && GridPos.Y > 2)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Errore: Devi piazzare nelle tue prime 3 righe in basso!"));
			Players[CurrentPlayer]->OnTurn();
			return;
		}
		if (CurrentPlayer == 1 && GridPos.Y < FieldSize - 3)
		{
			Players[CurrentPlayer]->OnTurn();
			return;
		}

		FVector Location = TargetTile->GetActorLocation();
		Location.Z += 60.0f; // Alziamo leggermente l'unità rispetto al terreno

		ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(UnitToSpawn, Location, FRotator::ZeroRotator);
		if (NewUnit)
		{
			NewUnit->PlayerOwner = CurrentPlayer;
			NewUnit->CurrentGridPosition = GridPos;

			if (CurrentPlayer == 0) Player0Units.Add(NewUnit);
			else Player1Units.Add(NewUnit);

			TargetTile->SetTileStatus(CurrentPlayer, ETileStatus::OCCUPIED);
			UnitsPlaced++;

			UE_LOG(LogTemp, Warning, TEXT("[Piazzamento] Giocatore %d ha piazzato un'unita' in (%.0f, %.0f)"), CurrentPlayer, GridPos.X, GridPos.Y);
		}

		if (Player0UnitsPlaced == 2 && Player1UnitsPlaced == 2)
		{
			CurrentGameState = EGameState::Playing;

			ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetWorld()->GetFirstPlayerController());
			if (PC && PC->ActionWidgetInstance)
			{
				PC->ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			}

			CurrentPlayer = StartingPlayer;
			Players[CurrentPlayer]->OnTurn();
			return;
		}

		TurnNextPlayer();
	}
	else if (CurrentGameState == EGameState::Playing)
	{
		TurnNextPlayer();
	}
}

int32 ATTT_GameMode::GetNextPlayer(int32 Player)
{
	Player++;
	if (!Players.IsValidIndex(Player)) Player = 0;
	return Player;
}

void ATTT_GameMode::TurnNextPlayer()
{
	MoveCounter += 1;
	CurrentPlayer = GetNextPlayer(CurrentPlayer);

	TArray<ABaseUnit*> CurrentUnits = (CurrentPlayer == 0) ? Player0Units : Player1Units;
	for (ABaseUnit* Unit : CurrentUnits)
	{
		if (Unit) Unit->bHasActedThisTurn = false;
	}
	Players[CurrentPlayer]->OnTurn();
}

void ATTT_GameMode::ClearTileHighlights()
{
	if (!GField) return;
	for (ATile* Tile : GField->TileArray)
	{
		if (Tile) Tile->SetTileHighlight(false, FLinearColor::Black);
	}
}

ABaseUnit* ATTT_GameMode::GetUnitAtGridPosition(FVector2D GridPos)
{
	for (ABaseUnit* Unit : Player0Units)
	{
		if (Unit && FMath::IsNearlyEqual(Unit->CurrentGridPosition.X, GridPos.X) && FMath::IsNearlyEqual(Unit->CurrentGridPosition.Y, GridPos.Y)) return Unit;
	}
	for (ABaseUnit* Unit : Player1Units)
	{
		if (Unit && FMath::IsNearlyEqual(Unit->CurrentGridPosition.X, GridPos.X) && FMath::IsNearlyEqual(Unit->CurrentGridPosition.Y, GridPos.Y)) return Unit;
	}
	return nullptr;
}

TArray<FVector2D> ATTT_GameMode::GetReachableCells(FVector2D StartGridPos, int32 MovementRange)
{
	TArray<FVector2D> ReachableCells;
	if (!GField) return ReachableCells;

	auto GetTileSafely = [&](FVector2D Pos) -> ATile* {
		int32 TargetX = FMath::RoundToInt(Pos.X);
		int32 TargetY = FMath::RoundToInt(Pos.Y);

		for (ATile* T : GField->TileArray) {
			if (T && FMath::RoundToInt(T->GetGridPosition().X) == TargetX && FMath::RoundToInt(T->GetGridPosition().Y) == TargetY) {
				return T;
			}
		}
		return nullptr;
		};

	ATile* StartTile = GetTileSafely(StartGridPos);
	if (!StartTile) return ReachableCells;

	TArray<TPair<FVector2D, int32>> Frontier;
	TArray<FVector2D> Visited;
	TArray<int32> VisitedCosts;

	Frontier.Add(TPair<FVector2D, int32>(StartGridPos, 0));
	Visited.Add(StartGridPos);
	VisitedCosts.Add(0);

	FVector2D Directions[4] = { FVector2D(0, 1), FVector2D(0, -1), FVector2D(1, 0), FVector2D(-1, 0) };

	while (Frontier.Num() > 0)
	{
		FVector2D CurrentPos = Frontier[0].Key;
		int32 CurrentCost = Frontier[0].Value;
		Frontier.RemoveAt(0);

		ATile* CurrentTile = GetTileSafely(CurrentPos);
		if (!CurrentTile) continue;

		for (int i = 0; i < 4; ++i)
		{
			FVector2D NextPos = CurrentPos + Directions[i];

			if (NextPos.X < 0 || NextPos.X >= FieldSize || NextPos.Y < 0 || NextPos.Y >= FieldSize) continue;

			ATile* NextTile = GetTileSafely(NextPos);
			if (!NextTile) continue;

			// 1. L'acqua è invalicabile (ElevationLevel == 0)
			if (NextTile->ElevationLevel == 0) continue;

			// 2. Torri e Nemici sono Ostacoli
			bool bIsStartPos = (FMath::RoundToInt(NextPos.X) == FMath::RoundToInt(StartGridPos.X)) &&
				(FMath::RoundToInt(NextPos.Y) == FMath::RoundToInt(StartGridPos.Y));

			if (NextTile->GetTileStatus() != ETileStatus::EMPTY && !bIsStartPos) continue;

			// 3. Movimento in salita costa 2! Movimento in piano o discesa costa 1.
			int32 MoveCost = (NextTile->ElevationLevel > CurrentTile->ElevationLevel) ? 2 : 1;
			int32 NewCost = CurrentCost + MoveCost;

			if (NewCost > MovementRange) continue;

			bool bAlreadyVisited = false;
			for (int32 v = 0; v < Visited.Num(); ++v)
			{
				if (FMath::RoundToInt(Visited[v].X) == FMath::RoundToInt(NextPos.X) &&
					FMath::RoundToInt(Visited[v].Y) == FMath::RoundToInt(NextPos.Y))
				{
					bAlreadyVisited = true;
					if (NewCost < VisitedCosts[v])
					{
						VisitedCosts[v] = NewCost;
						bAlreadyVisited = false;
					}
					break;
				}
			}

			if (!bAlreadyVisited)
			{
				Visited.Add(NextPos);
				VisitedCosts.Add(NewCost);
				Frontier.Add(TPair<FVector2D, int32>(NextPos, NewCost));

				bool bInReachable = false;
				for (FVector2D R : ReachableCells)
				{
					if (FMath::RoundToInt(R.X) == FMath::RoundToInt(NextPos.X) && FMath::RoundToInt(R.Y) == FMath::RoundToInt(NextPos.Y))
					{
						bInReachable = true; break;
					}
				}
				if (!bInReachable) ReachableCells.Add(NextPos);
			}
		}
	}
	return ReachableCells;
}
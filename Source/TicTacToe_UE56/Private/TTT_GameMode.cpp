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

	//timer di vittoria [inizializzo a 0]
	Player0WinTimer = 0;
	Player1WinTimer = 0;

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

	//LANCIO MONETA
	CurrentGameState = EGameState::CoinFlip;

	FString CoinMessage = FString::Printf(TEXT("LANCIO DELLA MONETA...\nInizia il Giocatore %d!"), CurrentPlayer + 1);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Black, CoinMessage, true, FVector2D(1.0f, 1.0f));
	UE_LOG(LogTemp, Warning, TEXT("[Setup] %s"), *CoinMessage);

	//attesa 2 secondi x lettura
	FTimerHandle CoinFlipTimer;
	GetWorld()->GetTimerManager().SetTimer(CoinFlipTimer, [this]() {
		CurrentGameState = EGameState::Placement;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("FASE DI PIAZZAMENTO: Schiera le tue unita'!"), true, FVector2D(1.0f, 1.0f));
		if (Players.IsValidIndex(CurrentPlayer)) Players[CurrentPlayer]->OnTurn();
		}, 2.0f, false);
}

void ATTT_GameMode::SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition)
{
	if (IsGameOver || PlayerNumber != CurrentPlayer) return;

	FVector2D GridPosRaw = GField->GetXYPositionByRelativeLocation(SpawnPosition);
	FVector2D GridPos(FMath::RoundToDouble(GridPosRaw.X), FMath::RoundToDouble(GridPosRaw.Y));

	if (CurrentGameState == EGameState::Placement)
	{
		int32& UnitsPlaced = (CurrentPlayer == 0) ? Player0UnitsPlaced : Player1UnitsPlaced;

		//CLASS PER 4 BLUEPRINT
		TSubclassOf<ABaseUnit> UnitToSpawn = nullptr;
		if (CurrentPlayer == 0)
		{
			//Player -> usa BP_Sniper & BP_Brawler
			UnitToSpawn = (UnitsPlaced == 0) ? SniperClass : BrawlerClass;
		}
		else
		{
			// IA -> usa BP_SniperAI e BP_BrawlerAI
			UnitToSpawn = (UnitsPlaced == 0) ? SniperAIClass : BrawlerAIClass;
		}

		if (!UnitToSpawn || !GField) return;

		ATile* TargetTile = nullptr;

		for (ATile* Tile : GField->TileArray)
		{
			if (Tile && FMath::IsNearlyEqual(Tile->GetGridPosition().X, GridPos.X) && FMath::IsNearlyEqual(Tile->GetGridPosition().Y, GridPos.Y))
			{
				TargetTile = Tile;
				break;
			}
		}

		//l'acqua non calpestabile!
		if (TargetTile && TargetTile->ElevationLevel == 0)
		{
			if (CurrentPlayer == 0) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Errore: L'Acqua non e' calpestabile!"));
			else
			{
				FTimerHandle RetryTimer;
				GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]() {
					if (Players.IsValidIndex(CurrentPlayer)) Players[CurrentPlayer]->OnTurn();
					}, 0.1f, false);
			}
			return;
		}

		// Controllo ostacoli fisici (Altre unità o Torri)
		if (!TargetTile || TargetTile->GetTileStatus() != ETileStatus::EMPTY)
		{
			if (CurrentPlayer == 0) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Errore: Cella occupata da una Torre o Unita'!"));
			else
			{
				FTimerHandle RetryTimer;
				GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]() {
					if (Players.IsValidIndex(CurrentPlayer)) Players[CurrentPlayer]->OnTurn();
					}, 0.1f, false);
			}
			return;
		}

		// Controllo validità aree di schieramento
		if (CurrentPlayer == 0 && GridPos.Y > 2)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Errore: Devi piazzare nelle tue prime 3 righe in basso!"));
			return;
		}
		if (CurrentPlayer == 1 && GridPos.Y < FieldSize - 3)
		{
			FTimerHandle RetryTimer;
			GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]() {
				if (Players.IsValidIndex(CurrentPlayer)) Players[CurrentPlayer]->OnTurn();
				}, 0.1f, false);
			return;
		}

		FVector Location = TargetTile->GetActorLocation();
		Location.Z += 60.0f;

		ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(UnitToSpawn, Location, FRotator::ZeroRotator);
		if (NewUnit)
		{
			NewUnit->PlayerOwner = CurrentPlayer;
			NewUnit->CurrentGridPosition = GridPos;

			//Salvataggio della posizione iniziale x il respawn
			NewUnit->InitialGridPosition = GridPos;

			if (CurrentPlayer == 0) Player0Units.Add(NewUnit);
			else Player1Units.Add(NewUnit);

			TargetTile->SetTileStatus(CurrentPlayer, ETileStatus::OCCUPIED);
			UnitsPlaced++;
		}

		if (Player0UnitsPlaced == 2 && Player1UnitsPlaced == 2)
		{
			CurrentGameState = EGameState::Playing;

			ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetWorld()->GetFirstPlayerController());
			if (PC && PC->ActionWidgetInstance) PC->ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);

			CurrentPlayer = StartingPlayer;
			Players[CurrentPlayer]->OnTurn();
			return;
		}

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
	if (CurrentGameState == EGameState::Playing)
	{
		EvaluateTowers();
		if (IsGameOver) return;
	}

	MoveCounter += 1;
	CurrentPlayer = GetNextPlayer(CurrentPlayer);

	TArray<ABaseUnit*> CurrentUnits = (CurrentPlayer == 0) ? Player0Units : Player1Units;
	for (ABaseUnit* Unit : CurrentUnits)
	{
		if (Unit) Unit->bHasActedThisTurn = false;
	}
	Players[CurrentPlayer]->OnTurn();
}

void ATTT_GameMode::EvaluateTowers()
{
	if (IsGameOver || !GField) return;

	if (TowerStates.Num() == 0)
	{
		for (ATile* Tile : GField->TileArray)
		{
			if (Tile && Tile->Tags.Contains(FName("TowerTile")))
			{
				TowerStates.Add(Tile->GetGridPosition(), -1);
			}
		}
	}

	for (auto& TowerPair : TowerStates)
	{
		FVector2D TowerPos = TowerPair.Key;
		int32 CurrentState = TowerPair.Value;

		bool bP0InRange = false;
		bool bP1InRange = false;

		for (ABaseUnit* Unit : Player0Units)
		{
			if (Unit && FMath::Max(FMath::Abs(Unit->CurrentGridPosition.X - TowerPos.X), FMath::Abs(Unit->CurrentGridPosition.Y - TowerPos.Y)) <= 2)
			{
				bP0InRange = true; break;
			}
		}

		for (ABaseUnit* Unit : Player1Units)
		{
			if (Unit && FMath::Max(FMath::Abs(Unit->CurrentGridPosition.X - TowerPos.X), FMath::Abs(Unit->CurrentGridPosition.Y - TowerPos.Y)) <= 2)
			{
				bP1InRange = true; break;
			}
		}

		if (bP0InRange && !bP1InRange)
		{
			TowerStates[TowerPos] = 0; // Stato B: P0
			UE_LOG(LogTemp, Warning, TEXT("[Macchina Stati] Torre in (%.0f, %.0f) controllata da UMANO (P0)."), TowerPos.X, TowerPos.Y);
		}
		else if (bP1InRange && !bP0InRange)
		{
			TowerStates[TowerPos] = 1; // Stato B: P1
			UE_LOG(LogTemp, Warning, TEXT("[Macchina Stati] Torre in (%.0f, %.0f) controllata da IA (P1)."), TowerPos.X, TowerPos.Y);
		}
		else if (bP0InRange && bP1InRange)
		{
			TowerStates[TowerPos] = 2; // Stato C: Contesa
			UE_LOG(LogTemp, Error, TEXT("[Macchina Stati] Torre in (%.0f, %.0f) in CONTESA! (Unita' multiple in range)."), TowerPos.X, TowerPos.Y);
		}
	}

	UpdateTowerVisuals();

	int32 P0Score = 0;
	int32 P1Score = 0;

	// In stato di Contesa (Value == 2) nessun giocatore guadagna il pt
	for (auto& TowerPair : TowerStates)
	{
		if (TowerPair.Value == 0) P0Score++;
		else if (TowerPair.Value == 1) P1Score++;
	}

	UE_LOG(LogTemp, Warning, TEXT("[TORRI] P0: %d | P1: %d"), P0Score, P1Score);

	//fine tuo turno (1), Fine turno IA (2), Fine del tuo prossimo turno (3). = 2 Turni tuoi completi
	if (P0Score >= 2) Player0WinTimer++;
	else Player0WinTimer = 0;

	if (P1Score >= 2) Player1WinTimer++;
	else Player1WinTimer = 0;

	UE_LOG(LogTemp, Warning, TEXT("[VITTORIA] WinTimer Umano (P0): %d/3 | WinTimer IA (P1): %d/3"), Player0WinTimer, Player1WinTimer);

	// Esecuzione Win
	if (Player0WinTimer >= 3) EndGame(0);
	else if (Player1WinTimer >= 3) EndGame(1);
}

void ATTT_GameMode::UpdateTowerVisuals()
{
	if (!GField || !GField->TowerClass) return;

	for (TActorIterator<AActor> It(GetWorld(), GField->TowerClass); It; ++It)
	{
		AActor* TowerActor = *It;
		FVector2D GridPos = GField->GetXYPositionByRelativeLocation(TowerActor->GetActorLocation());

		if (TowerStates.Contains(GridPos))
		{
			int32 State = TowerStates[GridPos];

			TArray<UMeshComponent*> Meshes;
			TowerActor->GetComponents<UMeshComponent>(Meshes);
			for (UMeshComponent* Mesh : Meshes)
			{
				UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0);
				if (MID)
				{
					FLinearColor C1 = FLinearColor::White; //Neutrale => Bianco
					FLinearColor C2 = FLinearColor::White;
					float IsContested = 0.0f; // 0 = Fisso, 1 = Lampeggia (Contesa)
					FString ColorLogName = TEXT("BIANCO (Neutrale)");

					if (State == 0)
					{
						C1 = FLinearColor::Black;
						ColorLogName = TEXT("NERO (Umano P0)");
					}
					else if (State == 1)
					{
						C1 = FLinearColor(1.0f, 0.4f, 0.7f, 1.0f);
						ColorLogName = TEXT("PINK (IA P1)");
					}
					else if (State == 2) // Contesa -> Lampeggia tra Nero (P0) e Pink (P1)
					{
						C1 = FLinearColor::Black;                  // Umano
						C2 = FLinearColor(1.0f, 0.4f, 0.7f, 1.0f); // IA
						IsContested = 1.0f;                        // Attiva il mix
						ColorLogName = TEXT("LAMPEGGIANTE NERO/PINK (Contesa)");
					}

					UE_LOG(LogTemp, Log, TEXT("[Visuals] Rendering Torre (%.0f, %.0f) -> Colore inviato alla GPU: %s"), GridPos.X, GridPos.Y, *ColorLogName);

					MID->SetVectorParameterValue(FName("BaseColor"), C1);
					MID->SetVectorParameterValue(FName("Color2"), C2);
					MID->SetScalarParameterValue(FName("IsContested"), IsContested);
				}
			}
		}
	}
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

			if (NextTile->ElevationLevel == 0) continue;

			bool bIsStartPos = (FMath::RoundToInt(NextPos.X) == FMath::RoundToInt(StartGridPos.X)) &&
				(FMath::RoundToInt(NextPos.Y) == FMath::RoundToInt(StartGridPos.Y));

			if (NextTile->GetTileStatus() != ETileStatus::EMPTY && !bIsStartPos) continue;

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

// ALGORITMO A* PER TROVARE IL PERCORSO min cellXcell
TArray<FVector2D> ATTT_GameMode::FindPath(FVector2D StartGridPos, FVector2D TargetGridPos)
{
	TArray<FVector2D> Path;
	if (!GField) return Path;

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

	TMap<FIntPoint, FIntPoint> CameFrom;
	TMap<FIntPoint, int32> CostSoFar;
	TArray<FIntPoint> Frontier;

	FIntPoint StartPoint(FMath::RoundToInt(StartGridPos.X), FMath::RoundToInt(StartGridPos.Y));
	FIntPoint TargetPoint(FMath::RoundToInt(TargetGridPos.X), FMath::RoundToInt(TargetGridPos.Y));

	Frontier.Add(StartPoint);
	CostSoFar.Add(StartPoint, 0);

	FIntPoint Directions[4] = { FIntPoint(0, 1), FIntPoint(0, -1), FIntPoint(1, 0), FIntPoint(-1, 0) };

	while (Frontier.Num() > 0)
	{
		Frontier.Sort([&CostSoFar](const FIntPoint& A, const FIntPoint& B) {
			return CostSoFar[A] < CostSoFar[B];
			});

		FIntPoint Current = Frontier[0];
		Frontier.RemoveAt(0);

		if (Current == TargetPoint) break;

		ATile* CurrentTile = GetTileSafely(FVector2D(Current.X, Current.Y));
		if (!CurrentTile) continue;

		for (int i = 0; i < 4; ++i)
		{
			FIntPoint Next = Current + Directions[i];
			ATile* NextTile = GetTileSafely(FVector2D(Next.X, Next.Y));

			if (!NextTile || NextTile->ElevationLevel == 0) continue;
			if (NextTile->GetTileStatus() != ETileStatus::EMPTY && Next != TargetPoint) continue;

			int32 MoveCost = (NextTile->ElevationLevel > CurrentTile->ElevationLevel) ? 2 : 1;
			int32 NewCost = CostSoFar[Current] + MoveCost;

			if (!CostSoFar.Contains(Next) || NewCost < CostSoFar[Next])
			{
				CostSoFar.Add(Next, NewCost);
				CameFrom.Add(Next, Current);
				Frontier.Add(Next);
			}
		}
	}

	if (!CameFrom.Contains(TargetPoint)) return Path;

	FIntPoint CurrentPathNode = TargetPoint;
	while (CurrentPathNode != StartPoint)
	{
		Path.Insert(FVector2D(CurrentPathNode.X, CurrentPathNode.Y), 0);
		CurrentPathNode = CameFrom[CurrentPathNode];
	}

	return Path;
}

void ATTT_GameMode::EndGame(int32 WinnerPlayer)
{
	IsGameOver = true;
	CurrentGameState = EGameState::GameOver;
	ClearTileHighlights();

	FString WinnerName = (WinnerPlayer == 0) ? TEXT("L'UMANO (P0)") : TEXT("L'IA (P1)");
	FString WinMsg = FString::Printf(TEXT("[GAME OVER] %s HA CONQUISTATO LE TORRI E VINTO LA PARTITA"), *WinnerName);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, WinMsg, true, FVector2D(1.0f, 1.0f));
	UE_LOG(LogTemp, Error, TEXT("%s"), *WinMsg);

	if (GameOverWidgetClass)
	{
		GameOverWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
		if (GameOverWidgetInstance)
		{
			GameOverWidgetInstance->AddToViewport(9999);
		}
	}
}
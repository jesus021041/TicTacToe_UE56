// Fill out your copyright notice in the Description page of Project Settings.

#include "TTT_RandomPlayer.h"
#include "TTT_GameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Tile.h"

// Sets default values
ATTT_RandomPlayer::ATTT_RandomPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATTT_RandomPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void ATTT_RandomPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(AI_TurnTimerHandle);
}

void ATTT_RandomPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Attende la fine camminata
	if (bIsMovingUnit && CurrentCommandUnit)
	{
		if (!CurrentCommandUnit->bIsMoving)
		{
			bIsMovingUnit = false;
			ExecuteAIAttack(); // Finito il movimento -> passa all'att
		}
	}
}

void ATTT_RandomPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATTT_RandomPlayer::OnTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("[IA] E' il mio turno! Analizzo la situazione..."));

	// Simulazione di tempo reazione => 1.5 secondi
	GetWorld()->GetTimerManager().SetTimer(AI_TurnTimerHandle, [this]()
		{
			ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
			if (!GameMode || GameMode->IsGameOver) return;

			if (GameMode->CurrentGameState == EGameState::Placement)
			{
				UE_LOG(LogTemp, Warning, TEXT("[IA] Cerco una casella valida per lo schieramento..."));

				TArray<ATile*> ValidPlacementTiles;

				// L'IA cerca solo le celle legali: Y >= (Size-3), No Acqua, Vuote, No Torri.
				for (ATile* Tile : GameMode->GField->TileArray)
				{
					if (Tile &&
						Tile->GetGridPosition().Y >= (GameMode->FieldSize - 3) &&
						Tile->ElevationLevel > 0 &&
						Tile->GetTileStatus() == ETileStatus::EMPTY &&
						!Tile->Tags.Contains(FName("TowerTile")))
					{
						ValidPlacementTiles.Add(Tile);
					}
				}

				if (ValidPlacementTiles.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, ValidPlacementTiles.Num() - 1);
					ATile* ChosenTile = ValidPlacementTiles[RandomIndex];

					UE_LOG(LogTemp, Warning, TEXT("[IA] Casella trovata in (%.0f, %.0f). Piazzo l'unita'."), ChosenTile->GetGridPosition().X, ChosenTile->GetGridPosition().Y);

					GameMode->SetCellSign(PlayerNumber, ChosenTile->GetActorLocation());
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[IA] IMPOSSIBILE PIAZZARE: Nessuna cella valida trovata. Passo il turno."));
					GameMode->TurnNextPlayer();
				}
			}
			else if (GameMode->CurrentGameState == EGameState::Playing)
			{
				UE_LOG(LogTemp, Warning, TEXT("[IA] Gioco: Calcolo Mossa con A*..."));

				//I. Trova un'unità dell'IA che non si è ancora mossa
				ABaseUnit* UnitToCommand = nullptr;
				for (ABaseUnit* Unit : GameMode->Player1Units)
				{
					if (Unit && !Unit->bHasActedThisTurn)
					{
						UnitToCommand = Unit;
						break;
					}
				}

				// Se tutte le unità hanno agito, passa il turno
				if (!UnitToCommand)
				{
					UE_LOG(LogTemp, Warning, TEXT("[IA] Tutte le mie unita' hanno agito -> passo "));
					GameMode->TurnNextPlayer();
					return;
				}

				// Salviamo l'unita' attuale per farla gestire dal Tick e ExecuteAIAttack
				CurrentCommandUnit = UnitToCommand;

				//II. logica del BRAIN: Trova il bersaglio!
				FVector2D TargetGridPos = FVector2D(-1, -1);
				float MinDist = MAX_FLT;

				// A. Dai priorità alle torri non ancora controllate dall'IA (Stato != 1)
				for (auto& TowerPair : GameMode->TowerStates)
				{
					if (TowerPair.Value != 1)
					{
						float Dist = FVector2D::Distance(UnitToCommand->CurrentGridPosition, TowerPair.Key);
						if (Dist < MinDist)
						{
							MinDist = Dist;
							TargetGridPos = TowerPair.Key;
						}
					}
				}

				// B. Se ho già tutte le torri, pt al nemico + vicino
				if (TargetGridPos == FVector2D(-1, -1))
				{
					for (ABaseUnit* Enemy : GameMode->Player0Units)
					{
						if (Enemy && Enemy->HealthPoints > 0)
						{
							float Dist = FVector2D::Distance(UnitToCommand->CurrentGridPosition, Enemy->CurrentGridPosition);
							if (Dist < MinDist)
							{
								MinDist = Dist;
								TargetGridPos = Enemy->CurrentGridPosition;
							}
						}
					}
				}

				//III. Esegui il Pathfinding A* verso il bersaglio scelto
				if (TargetGridPos != FVector2D(-1, -1))
				{
					int32 ActualRange = UnitToCommand->MovementRange;
					if (ActualRange <= 0) ActualRange = UnitToCommand->GetClass()->GetName().Contains(TEXT("Sniper")) ? 4 : 6;

					TArray<FVector2D> IdealPath = GameMode->FindPath(UnitToCommand->CurrentGridPosition, TargetGridPos);
					TArray<FVector2D> ReachableCells = GameMode->GetReachableCells(UnitToCommand->CurrentGridPosition, ActualRange);

					FVector2D BestMovePos = UnitToCommand->CurrentGridPosition;
					TArray<FVector2D> PathToTake;

					//Se non abbiamo un IdealPath completo ma ci sono celle raggiungibili, ci avviciniamo al nemico
					if (IdealPath.Num() == 0 && ReachableCells.Num() > 0)
					{
						float ClosestDistToTarget = MAX_FLT;
						for (FVector2D ReachableCell : ReachableCells)
						{
							float D = FMath::Abs(ReachableCell.X - TargetGridPos.X) + FMath::Abs(ReachableCell.Y - TargetGridPos.Y);
							if (D < ClosestDistToTarget)
							{
								ClosestDistToTarget = D;
								BestMovePos = ReachableCell;
							}
						}
						PathToTake = GameMode->FindPath(UnitToCommand->CurrentGridPosition, BestMovePos);
					}
					else if (IdealPath.Num() > 0)
					{
						for (int32 i = IdealPath.Num() - 1; i >= 0; i--)
						{
							if (ReachableCells.Contains(IdealPath[i]))
							{
								BestMovePos = IdealPath[i];
								for (int32 j = 0; j <= i; j++) PathToTake.Add(IdealPath[j]);
								break;
							}
						}
					}

					if (BestMovePos != UnitToCommand->CurrentGridPosition)
					{
						UE_LOG(LogTemp, Warning, TEXT("[IA] Mi muovo verso il bersaglio. Arrivo in (%.0f, %.0f)"), BestMovePos.X, BestMovePos.Y);

						//ACTION LOG -> UMANO MOVIMENTO
						FString UType = GameMode->GetUnitShortName(UnitToCommand);
						FString StartCell = GameMode->GetCellName(UnitToCommand->CurrentGridPosition);
						FString EndCell = GameMode->GetCellName(BestMovePos);
						FString MoveLog = FString::Printf(TEXT("AI: %s %s -> %s"), *UType, *StartCell, *EndCell);

						UE_LOG(LogTemp, Warning, TEXT("[ActionLog] %s"), *MoveLog);
						GameMode->AddToActionLog(MoveLog, FLinearColor::White);

						//HIGHLIGHT AI
						GameMode->ClearTileHighlights();
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && PathToTake.Contains(Tile->GetGridPosition()))
							{
								Tile->SetTileHighlight(true, FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
							}
						}

						// Svuoto la cella di origine
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(UnitToCommand->CurrentGridPosition.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(UnitToCommand->CurrentGridPosition.Y))
							{
								Tile->SetTileStatus(-1, ETileStatus::EMPTY);
								break;
							}
						}

						UnitToCommand->CurrentGridPosition = BestMovePos;

						// Occupo la cella di destinazione
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(BestMovePos.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(BestMovePos.Y))
							{
								Tile->SetTileStatus(1, ETileStatus::OCCUPIED);
								break;
							}
						}

						//MOVIMENTO
						UnitToCommand->MoveAlongPath(PathToTake);
						bIsMovingUnit = true;

						return;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[IA] Sono gia' in posizione o il percorso e' bloccato"));
						ExecuteAIAttack(); // L'IA ferma, attacca direttamente
						return;
					}
				}
				else
				{
					ExecuteAIAttack();
					return;
				}
			}

		}, 1.5f, false);
}

// L'Attacco è stato scorporato per poter essere chiamato dopo l'animazione di camminata
void ATTT_RandomPlayer::ExecuteAIAttack()
{
	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !CurrentCommandUnit) return;

	GameMode->ClearTileHighlights();

	//IV. Logica ATT dell'AI
	int32 ActualAttackRange = CurrentCommandUnit->AttackRange > 0 ? CurrentCommandUnit->AttackRange : (CurrentCommandUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);
	ABaseUnit* TargetEnemy = nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[IA] Cerco bersagli da attaccare... Il mio Raggio e': %d"), ActualAttackRange);

				// Troviamo il Tile su cui si trova l'IA per l'elevazione
				// Rispettaera regola
	ATile* MyTile = nullptr;
	for (ATile* Tile : GameMode->GField->TileArray)
	{
		if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(CurrentCommandUnit->CurrentGridPosition.X) &&
			FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(CurrentCommandUnit->CurrentGridPosition.Y))
		{
			MyTile = Tile;
			break;
		}
	}

	for (ABaseUnit* Enemy : GameMode->Player0Units)
	{
		if (Enemy && Enemy->HealthPoints > 0)
		{
			int32 DistX = FMath::Abs(FMath::RoundToInt(CurrentCommandUnit->CurrentGridPosition.X) - FMath::RoundToInt(Enemy->CurrentGridPosition.X));
			int32 DistY = FMath::Abs(FMath::RoundToInt(CurrentCommandUnit->CurrentGridPosition.Y) - FMath::RoundToInt(Enemy->CurrentGridPosition.Y));
			int32 Distance = DistX + DistY;

			if (Distance > 0 && Distance <= ActualAttackRange)
			{
				// Troviamo il Tile del nemico per leggerne l'elevazione
				ATile* EnemyTile = nullptr;
				for (ATile* Tile : GameMode->GField->TileArray)
				{
					if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(Enemy->CurrentGridPosition.X) &&
						FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(Enemy->CurrentGridPosition.Y))
					{
						EnemyTile = Tile;
						break;
					}
				}

				// Check elevazione
				if (MyTile && EnemyTile)
				{
					if (EnemyTile->ElevationLevel > MyTile->ElevationLevel)
					{
						UE_LOG(LogTemp, Warning, TEXT("[IA] Umano in (%.0f, %.0f) a tiro, ma l'Elevazione lo protegge! (Nemico: Liv %d > IA: Liv %d). Attacco no possibile."), Enemy->CurrentGridPosition.X, Enemy->CurrentGridPosition.Y, EnemyTile->ElevationLevel, MyTile->ElevationLevel);
						continue;
					}
				}

				TargetEnemy = Enemy;
				break;
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[IA] Umano in (%.0f, %.0f) ignorato. Distanza %d > Raggio %d"), Enemy->CurrentGridPosition.X, Enemy->CurrentGridPosition.Y, Distance, ActualAttackRange);
			}
		}
	}

	if (TargetEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IA] Bersaglio Umano in (%.0f, %.0f) valido => FUEGO"), TargetEnemy->CurrentGridPosition.X, TargetEnemy->CurrentGridPosition.Y);

		int32 Dmg = FMath::RandRange(CurrentCommandUnit->MinDamage, CurrentCommandUnit->MaxDamage);
		if (Dmg <= 0) Dmg = 1;

		TargetEnemy->HealthPoints -= Dmg;

		//ACTION LOG IA ATT
		FString UType = GameMode->GetUnitShortName(CurrentCommandUnit);
		FString TargetCell = GameMode->GetCellName(TargetEnemy->CurrentGridPosition);
		FString AtkLog = FString::Printf(TEXT("AI: %s %s %d"), *UType, *TargetCell, Dmg);

		UE_LOG(LogTemp, Warning, TEXT("[ActionLog] %s"), *AtkLog);
		GameMode->AddToActionLog(AtkLog, FLinearColor::Red);


		UE_LOG(LogTemp, Warning, TEXT("[IA] Inflitti %d danni. HP Umano restanti: %d"), Dmg, TargetEnemy->HealthPoints);

		if (TargetEnemy->HealthPoints <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[IA] HO SCONFITTO UN'UNITA' UMANA! (Eseguo il Respawn in base)"));

			// Svuoto la cella dove si trovava l'actor killato
			for (ATile* Tile : GameMode->GField->TileArray)
			{
				if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetEnemy->CurrentGridPosition.X) &&
					FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetEnemy->CurrentGridPosition.Y))
				{
					Tile->SetTileStatus(-1, ETileStatus::EMPTY);
					break;
				}
			}

			// Eseguo la Rinascita
			TargetEnemy->HealthPoints = TargetEnemy->MaxHP;
			TargetEnemy->CurrentGridPosition = TargetEnemy->InitialGridPosition;

			for (ATile* Tile : GameMode->GField->TileArray)
			{
				if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetEnemy->InitialGridPosition.X) &&
					FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetEnemy->InitialGridPosition.Y))
				{
					FVector SpawnLoc = Tile->GetActorLocation();
					SpawnLoc.Z += 60.f;
					TargetEnemy->SetActorLocation(SpawnLoc, false, nullptr, ETeleportType::TeleportPhysics);
					Tile->SetTileStatus(TargetEnemy->PlayerOwner, ETileStatus::OCCUPIED);
					break;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[IA] Nessun bersaglio valido a tiro per questa unita'."));
	}

	// Marca l'unità come "mossa"
	CurrentCommandUnit->bHasActedThisTurn = true;
	CurrentCommandUnit = nullptr;

				// Richiama OnTurn in modo ricorsivo tra 0.8 secondi per comandare l'unità succe.
	GetWorld()->GetTimerManager().SetTimer(AI_TurnTimerHandle, [this]() {
		OnTurn();
		}, 0.8f, false);
}

void ATTT_RandomPlayer::OnWin()
{
}

void ATTT_RandomPlayer::OnLose()
{
}
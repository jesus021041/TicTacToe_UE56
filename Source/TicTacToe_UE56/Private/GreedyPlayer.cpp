// Fill out your copyright notice in the Description page of Project Settings.

#include "GreedyPlayer.h"
#include "TTT_GameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Tile.h"

AGreedyPlayer::AGreedyPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGreedyPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void AGreedyPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(AI_TurnTimerHandle);
}

void AGreedyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMovingUnit && CurrentCommandUnit)
	{
		if (!CurrentCommandUnit->bIsMoving)
		{
			bIsMovingUnit = false;
			ExecuteAIAttack();
		}
	}
}

void AGreedyPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

//ALGORITMO EURISTICO (GREEDY BEST-FIRST)

TArray<FVector2D> AGreedyPlayer::FindGreedyPath(FVector2D Start, FVector2D Target, int32 MaxSteps)
{
	TArray<FVector2D> Path;
	FVector2D CurrentPos = Start;

	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !GameMode->GField) return Path;

	FVector2D Directions[4] = { FVector2D(0,1), FVector2D(0,-1), FVector2D(1,0), FVector2D(-1,0) };

	for (int32 Step = 0; Step < MaxSteps; ++Step)
	{
		FVector2D BestNextPos = CurrentPos;

		// Calcolo l'Euristica iniziale (Distanza in linea d'aria H)
		float MinDist = FVector2D::Distance(CurrentPos, Target);
		bool bFoundBetterMove = false;

		// Analizza solo i 4 + vicini
		for (int i = 0; i < 4; i++)
		{
			FVector2D Next = CurrentPos + Directions[i];

			//out mappa
			if (Next.X < 0 || Next.Y < 0 || Next.X >= GameMode->FieldSize || Next.Y >= GameMode->FieldSize) continue;

			ATile* NextTile = nullptr;
			for (ATile* T : GameMode->GField->TileArray)
			{
				if (T && FMath::RoundToInt(T->GetGridPosition().X) == FMath::RoundToInt(Next.X) &&
					FMath::RoundToInt(T->GetGridPosition().Y) == FMath::RoundToInt(Next.Y))
				{
					NextTile = T;
					break;
				}
			}

			// Ignora le caselle occupate
			if (!NextTile || NextTile->ElevationLevel == 0 || (NextTile->GetTileStatus() != ETileStatus::EMPTY && Next != Target)) continue;

			// Calcola la nuova Euristica H
			float DistToTarget = FVector2D::Distance(Next, Target);

			// SCELTA GREEDY -> prende solo la cella che ha la distanza min
			if (DistToTarget < MinDist)
			{
				MinDist = DistToTarget;
				BestNextPos = Next;
				bFoundBetterMove = true;
			}
		}

		if (bFoundBetterMove)
		{
			CurrentPos = BestNextPos;
			Path.Add(CurrentPos);

			//se è arrivato adiacente al bersaglio, si ferma
			if (CurrentPos == Target) break;
		}
		else
		{
			//l'algoritmo Greedy non sa tornare indietro.
			UE_LOG(LogTemp, Error, TEXT("[Greedy AI] L'Euristica mi ha bloccato mossa finita."));
			break;
		}
	}

	if (Path.Num() > 0 && Path.Last() == Target)
	{
		Path.RemoveAt(Path.Num() - 1);
	}

	return Path;
}

void AGreedyPlayer::OnTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("[Greedy AI] E' il mio turno! Inizializzazione..."));

	GetWorld()->GetTimerManager().SetTimer(AI_TurnTimerHandle, [this]()
		{
			ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
			if (!GameMode || GameMode->IsGameOver) return;

			if (GameMode->CurrentGameState == EGameState::Placement)
			{
				TArray<ATile*> ValidPlacementTiles;
				for (ATile* Tile : GameMode->GField->TileArray)
				{
					if (Tile && Tile->GetGridPosition().Y >= (GameMode->FieldSize - 3) &&
						Tile->ElevationLevel > 0 && Tile->GetTileStatus() == ETileStatus::EMPTY &&
						!Tile->Tags.Contains(FName("TowerTile")))
					{
						ValidPlacementTiles.Add(Tile);
					}
				}

				if (ValidPlacementTiles.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, ValidPlacementTiles.Num() - 1);
					GameMode->SetCellSign(PlayerNumber, ValidPlacementTiles[RandomIndex]->GetActorLocation());
				}
				else
				{
					GameMode->TurnNextPlayer();
				}
			}
			else if (GameMode->CurrentGameState == EGameState::Playing)
			{
				CurrentCommandUnit = nullptr;
				for (ABaseUnit* Unit : GameMode->Player1Units)
				{
					if (Unit && !Unit->bHasActedThisTurn)
					{
						CurrentCommandUnit = Unit;
						break;
					}
				}

				if (!CurrentCommandUnit)
				{
					GameMode->TurnNextPlayer();
					return;
				}

				FVector2D TargetGridPos = FVector2D(-1, -1);
				float MinDist = MAX_FLT;

				// Bersaglio primario: Le Torri
				for (auto& TowerPair : GameMode->TowerStates)
				{
					if (TowerPair.Value != 1)
					{
						float Dist = FVector2D::Distance(CurrentCommandUnit->CurrentGridPosition, TowerPair.Key);
						if (Dist < MinDist) { MinDist = Dist; TargetGridPos = TowerPair.Key; }
					}
				}

				// Bersaglio secondario: L'umano
				if (TargetGridPos == FVector2D(-1, -1))
				{
					for (ABaseUnit* Enemy : GameMode->Player0Units)
					{
						if (Enemy && Enemy->HealthPoints > 0)
						{
							float Dist = FVector2D::Distance(CurrentCommandUnit->CurrentGridPosition, Enemy->CurrentGridPosition);
							if (Dist < MinDist) { MinDist = Dist; TargetGridPos = Enemy->CurrentGridPosition; }
						}
					}
				}

				if (TargetGridPos != FVector2D(-1, -1))
				{
					int32 ActualRange = CurrentCommandUnit->MovementRange > 0 ? CurrentCommandUnit->MovementRange : (CurrentCommandUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 4 : 6);

					//Call al new entry: 'ALGORITMO GREEDY'
					TArray<FVector2D> PathToTake = FindGreedyPath(CurrentCommandUnit->CurrentGridPosition, TargetGridPos, ActualRange);

					if (PathToTake.Num() > 0)
					{
						FVector2D BestMovePos = PathToTake.Last();

						FString UType = GameMode->GetUnitShortName(CurrentCommandUnit);
						FString StartCell = GameMode->GetCellName(CurrentCommandUnit->CurrentGridPosition);
						FString EndCell = GameMode->GetCellName(BestMovePos);
						FString MoveLog = FString::Printf(TEXT("Greedy: %s %s -> %s"), *UType, *StartCell, *EndCell);

						UE_LOG(LogTemp, Warning, TEXT("[ActionLog] %s"), *MoveLog);
						GameMode->AddToActionLog(MoveLog, FLinearColor::White);

						GameMode->ClearTileHighlights();
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && PathToTake.Contains(Tile->GetGridPosition()))
							{
								Tile->SetTileHighlight(true, FLinearColor(0.0f, 1.0f, 1.0f, 1.0f)); // Azzurro per il Greedy
							}
						}

						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(CurrentCommandUnit->CurrentGridPosition.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(CurrentCommandUnit->CurrentGridPosition.Y))
							{
								Tile->SetTileStatus(-1, ETileStatus::EMPTY);
								break;
							}
						}

						CurrentCommandUnit->CurrentGridPosition = BestMovePos;
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(BestMovePos.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(BestMovePos.Y))
							{
								Tile->SetTileStatus(1, ETileStatus::OCCUPIED);
								break;
							}
						}

						CurrentCommandUnit->MoveAlongPath(PathToTake);
						bIsMovingUnit = true;
						return;
					}
					else
					{
						ExecuteAIAttack();
					}
				}
				else
				{
					ExecuteAIAttack();
				}
			}
		}, 1.5f, false);
}

void AGreedyPlayer::ExecuteAIAttack()
{
	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !CurrentCommandUnit) return;

	GameMode->ClearTileHighlights();

	int32 ActualAttackRange = CurrentCommandUnit->AttackRange > 0 ? CurrentCommandUnit->AttackRange : (CurrentCommandUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);
	ABaseUnit* TargetEnemy = nullptr;
	int32 FinalDistance = 0;

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

				if (MyTile && EnemyTile && EnemyTile->ElevationLevel > MyTile->ElevationLevel) continue;
				TargetEnemy = Enemy;
				FinalDistance = Distance;
				break;
			}
		}
	}

	if (TargetEnemy)
	{
		int32 Dmg = FMath::RandRange(CurrentCommandUnit->MinDamage, CurrentCommandUnit->MaxDamage);
		if (Dmg <= 0) Dmg = 1;
		TargetEnemy->HealthPoints -= Dmg;

		FString UType = GameMode->GetUnitShortName(CurrentCommandUnit);
		FString TargetCell = GameMode->GetCellName(TargetEnemy->CurrentGridPosition);
		FString AtkLog = FString::Printf(TEXT("Greedy AI: %s %s %d"), *UType, *TargetCell, Dmg);

		GameMode->AddToActionLog(AtkLog, FLinearColor::Red);

		if (TargetEnemy->HealthPoints <= 0)
		{
			for (ATile* Tile : GameMode->GField->TileArray)
			{
				if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetEnemy->CurrentGridPosition.X) &&
					FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetEnemy->CurrentGridPosition.Y))
				{
					Tile->SetTileStatus(-1, ETileStatus::EMPTY);
					break;
				}
			}

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
		else
		{
			// CONTRATTACCO DELL'IA
			bool bIsAttackerSniper = CurrentCommandUnit->GetClass()->GetName().Contains(TEXT("Sniper"));
			bool bIsTargetSniper = TargetEnemy->GetClass()->GetName().Contains(TEXT("Sniper"));
			bool bIsTargetBrawler = TargetEnemy->GetClass()->GetName().Contains(TEXT("Brawler"));

			if (bIsAttackerSniper)
			{
				if (bIsTargetSniper || (bIsTargetBrawler && FinalDistance == 1))
				{
					int32 CounterDmg = FMath::RandRange(1, 3);
					FVector2D MyGridPos = CurrentCommandUnit->CurrentGridPosition;
					CurrentCommandUnit->HealthPoints -= CounterDmg;

					FString CounterLog = FString::Printf(TEXT("HP: %s %s %d (Counter)"), *GameMode->GetUnitShortName(TargetEnemy), *GameMode->GetCellName(MyGridPos), CounterDmg);
					GameMode->AddToActionLog(CounterLog, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));

					if (CurrentCommandUnit->HealthPoints <= 0)
					{
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(MyGridPos.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(MyGridPos.Y))
							{
								Tile->SetTileStatus(-1, ETileStatus::EMPTY);
								break;
							}
						}

						CurrentCommandUnit->HealthPoints = CurrentCommandUnit->MaxHP;
						CurrentCommandUnit->CurrentGridPosition = CurrentCommandUnit->InitialGridPosition;

						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(CurrentCommandUnit->InitialGridPosition.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(CurrentCommandUnit->InitialGridPosition.Y))
							{
								FVector SpawnLoc = Tile->GetActorLocation();
								SpawnLoc.Z += 60.f;
								CurrentCommandUnit->SetActorLocation(SpawnLoc, false, nullptr, ETeleportType::TeleportPhysics);
								Tile->SetTileStatus(CurrentCommandUnit->PlayerOwner, ETileStatus::OCCUPIED);
								break;
							}
						}
					}
				}
			}
		}
	}

	CurrentCommandUnit->bHasActedThisTurn = true;
	CurrentCommandUnit = nullptr;

	GetWorld()->GetTimerManager().SetTimer(AI_TurnTimerHandle, [this]() {
		OnTurn();
		}, 0.8f, false);
}

void AGreedyPlayer::OnWin() {}
void AGreedyPlayer::OnLose() {}
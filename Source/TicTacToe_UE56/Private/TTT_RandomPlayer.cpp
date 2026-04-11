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
						if (Enemy)
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

					if (IdealPath.Num() > 0)
					{
						for (int32 i = IdealPath.Num() - 1; i >= 0; i--)
						{
							if (ReachableCells.Contains(IdealPath[i]))
							{
								BestMovePos = IdealPath[i];
								break;
							}
						}
					}

					if (BestMovePos != UnitToCommand->CurrentGridPosition)
					{
						UE_LOG(LogTemp, Warning, TEXT("[IA] Mi muovo verso il bersaglio. Arrivo in (%.0f, %.0f)"), BestMovePos.X, BestMovePos.Y);

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

						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(BestMovePos.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(BestMovePos.Y))
							{
								Tile->SetTileStatus(1, ETileStatus::OCCUPIED);
								FVector NewLoc = Tile->GetActorLocation();
								NewLoc.Z += 60.0f;
								UnitToCommand->SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
								break;
							}
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[IA] Sono gia' in posizione o il percorso e' bloccato"));
					}
				}

				//IV. Logica ATT dell'AI
				int32 ActualAttackRange = UnitToCommand->AttackRange > 0 ? UnitToCommand->AttackRange : (UnitToCommand->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);
				ABaseUnit* TargetEnemy = nullptr;

				UE_LOG(LogTemp, Warning, TEXT("[IA] Cerco bersagli da attaccare... Il mio Raggio e': %d"), ActualAttackRange);

				// Troviamo il Tile su cui si trova l'IA per l'elevazione
				// Rispettaera regola
				ATile* MyTile = nullptr;
				for (ATile* Tile : GameMode->GField->TileArray)
				{
					if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(UnitToCommand->CurrentGridPosition.X) &&
						FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(UnitToCommand->CurrentGridPosition.Y))
					{
						MyTile = Tile;
						break;
					}
				}

				for (ABaseUnit* Enemy : GameMode->Player0Units)
				{
					if (Enemy && Enemy->HealthPoints > 0)
					{
						int32 DistX = FMath::Abs(FMath::RoundToInt(UnitToCommand->CurrentGridPosition.X) - FMath::RoundToInt(Enemy->CurrentGridPosition.X));
						int32 DistY = FMath::Abs(FMath::RoundToInt(UnitToCommand->CurrentGridPosition.Y) - FMath::RoundToInt(Enemy->CurrentGridPosition.Y));
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

					int32 Dmg = FMath::RandRange(UnitToCommand->MinDamage, UnitToCommand->MaxDamage);
					if (Dmg <= 0) Dmg = 1;

					TargetEnemy->HealthPoints -= Dmg;

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
				UnitToCommand->bHasActedThisTurn = true;

				// Richiama OnTurn in modo ricorsivo tra 0.8 secondi per comandare l'unità succe.
				GetWorld()->GetTimerManager().SetTimer(AI_TurnTimerHandle, [this]() {
					OnTurn();
					}, 0.8f, false);
			}

		}, 1.5f, false);
}

void ATTT_RandomPlayer::OnWin()
{
}

void ATTT_RandomPlayer::OnLose()
{
}
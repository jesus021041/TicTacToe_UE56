// Fill out your copyright notice in the Description page of Project Settings.

#include "TTT_PlayerController.h"
#include "TTT_GameMode.h"
#include "Tile.h"
#include "Blueprint/UserWidget.h"
#include "GameField.h" 
#include "Sniper.h"
#include "Brawler.h"

ATTT_PlayerController::ATTT_PlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ATTT_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	SetInputMode(InputMode);

	if (ActionWidgetClass)
	{
		ActionWidgetInstance = CreateWidget<UUnitActionWidget>(this, ActionWidgetClass);
		if (ActionWidgetInstance)
		{
			ActionWidgetInstance->AddToViewport();
			ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ATTT_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATTT_PlayerController::OnLeftMouseClick);
}

// FUNZIONE TICK: ATTENDE LA FINE DELL'ANIMAZIONE DELLA PEDINA
void ATTT_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Controlla se la pedina ha finito l'animazione di camminata
	if (bIsMovingUnit && SelectedUnit)
	{
		if (!SelectedUnit->bIsMoving) // Questa variabile diventa false quando la pedina arriva a destinazione
		{
			FinalizeMovement();
		}
	}
}

// FUNZIONE CHIAMATA ALLA FINE DELL'ANIMAZIONE DI MOVIMENTO
void ATTT_PlayerController::FinalizeMovement()
{
	bIsMovingUnit = false;
	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !SelectedUnit) return;

	// 1. Aggiorna Logica e Griglia al traguardo
	SelectedUnit->CurrentGridPosition = TargetFinalGridPos;

	FName MovedTag = FName(*FString::Printf(TEXT("MovedInTurn_%d"), GameMode->MoveCounter));
	SelectedUnit->Tags.Add(MovedTag);

	bool bTowerConquered = false;

	for (ATile* Tile : GameMode->GField->TileArray)
	{
		if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetFinalGridPos.X) &&
			FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetFinalGridPos.Y))
		{
			Tile->SetTileStatus(SelectedUnit->PlayerOwner, ETileStatus::OCCUPIED);

			if (Tile->Tags.Contains(FName("TowerTile")))
			{
				bTowerConquered = true;
			}

			break;
		}
	}

	GameMode->ClearTileHighlights();
	CurrentActionState = EPlayerActionState::Idle;

	UE_LOG(LogTemp, Warning, TEXT("[Movimento] SPOSTAMENTO COMPLETATO su Cella (%.0f, %.0f)!"), TargetFinalGridPos.X, TargetFinalGridPos.Y);

	if (bTowerConquered)
	{
		if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		GameMode->EndGame(SelectedUnit->PlayerOwner);
		return;
	}

	// 2. Riattiviamo la UI per poter Attaccare o Passare il turno
	if (ActionWidgetInstance)
	{
		ActionWidgetInstance->UpdateUI(SelectedUnit);
		ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
}

void ATTT_PlayerController::OnLeftMouseClick()
{
	// Blocchiamo i click dell'utente finché la pedina sta camminando!
	if (bIsMovingUnit) return;

	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || GameMode->CurrentPlayer != 0) return;

	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		AActor* ClickedActor = HitResult.GetActor();

		// FASE I: PIAZZAMENTO
		if (GameMode->CurrentGameState == EGameState::Placement)
		{
			if (ClickedActor && ClickedActor->IsA(ATile::StaticClass()))
			{
				ATile* ClickedTile = Cast<ATile>(ClickedActor);
				FVector ExactRelativeLocation = GameMode->GField->GetRelativeLocationByXYPosition(
					FMath::RoundToInt(ClickedTile->GetGridPosition().X),
					FMath::RoundToInt(ClickedTile->GetGridPosition().Y));

				GameMode->SetCellSign(0, ExactRelativeLocation);
			}
		}
		// FASE II: GIOCO
		else if (GameMode->CurrentGameState == EGameState::Playing)
		{
			// CASO A: MOVIMENTO UMANO
			if (CurrentActionState == EPlayerActionState::SelectingMove && SelectedUnit)
			{
				if (ClickedActor && ClickedActor->IsA(ATile::StaticClass()))
				{
					ATile* ClickedTile = Cast<ATile>(ClickedActor);
					FVector2D TargetGridPos = ClickedTile->GetGridPosition();

					int32 ActualRange = SelectedUnit->MovementRange;
					if (ActualRange <= 0)
					{
						ActualRange = SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 4 : 6;
					}

					TArray<FVector2D> ValidCells = GameMode->GetReachableCells(SelectedUnit->CurrentGridPosition, ActualRange);

					bool bIsValidTarget = false;
					for (FVector2D ValidCell : ValidCells)
					{
						if (FMath::RoundToInt(ValidCell.X) == FMath::RoundToInt(TargetGridPos.X) &&
							FMath::RoundToInt(ValidCell.Y) == FMath::RoundToInt(TargetGridPos.Y))
						{
							bIsValidTarget = true;
							break;
						}
					}

					// Annulla se clicca sulla stessa cella
					if (FMath::RoundToInt(TargetGridPos.X) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) &&
						FMath::RoundToInt(TargetGridPos.Y) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y))
					{
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] Mossa annullata: hai cliccato sulla tua stessa cella."));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
						return;
					}

					FName MovedTag = FName(*FString::Printf(TEXT("MovedInTurn_%d"), GameMode->MoveCounter));
					if (SelectedUnit->Tags.Contains(MovedTag))
					{
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] Mossa Bloccata: Questa unita' si e' gia' mossa in questo turno!"));
						GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Questa unita' si e' gia' mossa!"));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
						return;
					}

					if (bIsValidTarget)
					{
						// Calcola il path da percorrere passo dopo passo! (Algoritmo A*)
						TArray<FVector2D> Path2D = GameMode->FindPath(SelectedUnit->CurrentGridPosition, TargetGridPos);

						if (Path2D.Num() > 0)
						{
							// REQUISITO 9: ACTION LOG UMANO MOVIMENTO
							FString UType = GameMode->GetUnitShortName(SelectedUnit);
							FString StartCell = GameMode->GetCellName(SelectedUnit->CurrentGridPosition);
							FString EndCell = GameMode->GetCellName(TargetGridPos);
							FString MoveLog = FString::Printf(TEXT("HP: %s %s -> %s"), *UType, *StartCell, *EndCell);

							UE_LOG(LogTemp, Warning, TEXT("[ActionLog] %s"), *MoveLog);
							GameMode->AddToActionLog(MoveLog, FLinearColor::White);

							for (ATile* Tile : GameMode->GField->TileArray)
							{
								if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) &&
									FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y))
								{
									Tile->SetTileStatus(-1, ETileStatus::EMPTY);
									break;
								}
							}

							TargetFinalGridPos = TargetGridPos;

							// Inizia la sequenza di camminata
							bIsMovingUnit = true;
							SelectedUnit->MoveAlongPath(Path2D);

							GameMode->ClearTileHighlights();
							CurrentActionState = EPlayerActionState::Idle;
							if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] Click su cella non valida (acqua OR occupata) o irragiungibile."));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Movimento] Mossa annullata per click fuori dalla griglia."));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;

					if (ActionWidgetInstance)
					{
						ActionWidgetInstance->UpdateUI(SelectedUnit);
						ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
					}
				}

				return;
			}

			// CASO C: STIAMO SELEZIONANDO CHI ATTACCARE
			if (CurrentActionState == EPlayerActionState::SelectingAttack && SelectedUnit)
			{
				//sicurezza anti doppio Attacco
				// Anche se riesci ad aprire il menu, l'ATT viene bloccato
				if (SelectedUnit->bHasActedThisTurn)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Attacco negato: l'unita' ha gia' attaccato in questo turno."));
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Questa unita' ha gia' attaccato!"));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;

					if (ActionWidgetInstance)
					{
						ActionWidgetInstance->UpdateUI(SelectedUnit);
						ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
					}
					return;
				}

				ABaseUnit* TargetUnit = Cast<ABaseUnit>(ClickedActor);

				if (TargetUnit && TargetUnit->PlayerOwner != SelectedUnit->PlayerOwner) // È un nemico?
				{
					// Identifichiamo il Tile sotto di noi e il Tile sotto al nemico.
					ATile* AttackerTile = nullptr;
					ATile* TargetTile = nullptr;

					for (ATile* Tile : GameMode->GField->TileArray)
					{
						if (!Tile) continue;

						if (FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) &&
							FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y))
						{
							AttackerTile = Tile;
						}

						if (FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetUnit->CurrentGridPosition.X) &&
							FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetUnit->CurrentGridPosition.Y))
						{
							TargetTile = Tile;
						}
					}

					if (AttackerTile && TargetTile)
					{
						//regola;il bersaglio NON PUÒ ESSERE ad un liv > del ns.
						if (TargetTile->ElevationLevel > AttackerTile->ElevationLevel)
						{
							UE_LOG(LogTemp, Error, TEXT("[Combattimento] ATTACCO NEGATO: Il nemico (Livello %d) e' piu' in alto di te (Livello %d)."), TargetTile->ElevationLevel, AttackerTile->ElevationLevel);
							GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("Attacco Fallito: Il bersaglio e' in una posizione sopraelevata, IRRANGIUNGIBILE!"));

							GameMode->ClearTileHighlights();
							CurrentActionState = EPlayerActionState::Idle;

							if (ActionWidgetInstance)
							{
								ActionWidgetInstance->UpdateUI(SelectedUnit);
								ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
							}
							return; //blocca il colpo
						}
					}

					int32 DistX = FMath::Abs(FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) - FMath::RoundToInt(TargetUnit->CurrentGridPosition.X));
					int32 DistY = FMath::Abs(FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y) - FMath::RoundToInt(TargetUnit->CurrentGridPosition.Y));
					int32 Distance = DistX + DistY;

					int32 ActualAttackRange = SelectedUnit->AttackRange > 0 ? SelectedUnit->AttackRange : (SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);

					if (Distance <= ActualAttackRange)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] BERSAGLIO COLPITO! Distanza: %d. Range: %d"), Distance, ActualAttackRange);

						FVector2D TargetGridPos = TargetUnit->CurrentGridPosition;

						// 1. DANNO AL BERSAGLIO
						int32 Dmg = FMath::RandRange(SelectedUnit->MinDamage, SelectedUnit->MaxDamage);
						TargetUnit->HealthPoints -= Dmg;

						//ACTION LOG -> UMANO ATT
						FString UType = GameMode->GetUnitShortName(SelectedUnit);
						FString TargetCell = GameMode->GetCellName(TargetGridPos);
						FString AtkLog = FString::Printf(TEXT("HP: %s %s %d"), *UType, *TargetCell, Dmg);

						UE_LOG(LogTemp, Warning, TEXT("[ActionLog] %s"), *AtkLog);
						GameMode->AddToActionLog(AtkLog, FLinearColor::Red);

						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Il nemico subisce %d danni. Salute rimasta: %d"), Dmg, TargetUnit->HealthPoints);

						// 2. CONTROLLO MORTE NEMICO, X DOPO RINASCERE
						if (TargetUnit->HealthPoints <= 0)
						{
							UE_LOG(LogTemp, Error, TEXT("[Combattimento] NEMICO RIDOTTO IN ATOMI!!!! Esecuzione Respawn..."));

							// Svuotiamo la cella in cui ha fatto il suo ciclo
							for (ATile* Tile : GameMode->GField->TileArray)
							{
								if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetGridPos.X) &&
									FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetGridPos.Y))
								{
									Tile->SetTileStatus(-1, ETileStatus::EMPTY);
									break;
								}
							}

							//rinascita: ripristina statistiche e coordinate d'inizio
							TargetUnit->HealthPoints = TargetUnit->MaxHP;
							TargetUnit->CurrentGridPosition = TargetUnit->InitialGridPosition;

							//alla ricerca della cella
							for (ATile* Tile : GameMode->GField->TileArray)
							{
								if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetUnit->InitialGridPosition.X) &&
									FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetUnit->InitialGridPosition.Y))
								{
									FVector SpawnLoc = Tile->GetActorLocation();
									SpawnLoc.Z += 60.f;
									TargetUnit->SetActorLocation(SpawnLoc, false, nullptr, ETeleportType::TeleportPhysics);
									Tile->SetTileStatus(TargetUnit->PlayerOwner, ETileStatus::OCCUPIED);
									break;
								}
							}
						}
						else
						{
							// 3. IL CONTRATTACCO ONLY SNIPER
							bool bIsAttackerSniper = SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper"));
							bool bIsTargetSniper = TargetUnit->GetClass()->GetName().Contains(TEXT("Sniper"));
							bool bIsTargetBrawler = TargetUnit->GetClass()->GetName().Contains(TEXT("Brawler"));

							if (bIsAttackerSniper)
							{
								if (bIsTargetSniper || (bIsTargetBrawler && Distance == 1))
								{
									UE_LOG(LogTemp, Warning, TEXT("[Combattimento] REGOLA CICALA: CONTRATTACCO INNESCATO SULLO SNIPER!"));

									int32 CounterDmg = FMath::RandRange(1, 3);
									FVector2D MyGridPos = SelectedUnit->CurrentGridPosition;
									SelectedUnit->HealthPoints -= CounterDmg;

									// ACTION LOG CONTRATTACCO
									FString CounterLog = FString::Printf(TEXT("AI: %s %s %d (Contrattacco)"), *GameMode->GetUnitShortName(TargetUnit), *GameMode->GetCellName(MyGridPos), CounterDmg);
									GameMode->AddToActionLog(CounterLog, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));

									UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Il tuo Sniper subisce %d danni da contrattacco. Salute rimasta: %d"), CounterDmg, SelectedUnit->HealthPoints);

									//rinascita in caso di morte x contrattaco(sniper)
									if (SelectedUnit->HealthPoints <= 0)
									{
										UE_LOG(LogTemp, Error, TEXT("[Combattimento] LA TUA UNITA' E' MORTA PER IL CONTRATTACCO! Esecuzione Respawn..."));

										for (ATile* Tile : GameMode->GField->TileArray)
										{
											if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(MyGridPos.X) &&
												FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(MyGridPos.Y))
											{
												Tile->SetTileStatus(-1, ETileStatus::EMPTY);
												break;
											}
										}

										SelectedUnit->HealthPoints = SelectedUnit->MaxHP;
										SelectedUnit->CurrentGridPosition = SelectedUnit->InitialGridPosition;

										for (ATile* Tile : GameMode->GField->TileArray)
										{
											if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(SelectedUnit->InitialGridPosition.X) &&
												FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(SelectedUnit->InitialGridPosition.Y))
											{
												FVector SpawnLoc = Tile->GetActorLocation();
												SpawnLoc.Z += 60.f;
												SelectedUnit->SetActorLocation(SpawnLoc, false, nullptr, ETeleportType::TeleportPhysics);
												Tile->SetTileStatus(SelectedUnit->PlayerOwner, ETileStatus::OCCUPIED);
												break;
											}
										}

										SelectedUnit = nullptr;
									}
								}
							}
						}

						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						//Chiudiamo l'azione e marchiamo l'unità
						if (SelectedUnit)
						{
							SelectedUnit->bHasActedThisTurn = true;
						}

						if (ActionWidgetInstance && SelectedUnit)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
						else if (ActionWidgetInstance)
						{
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
						}

					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Bersaglio FUORI RAGGIO! Distanza: %d. Il tuo Range: %d"), Distance, ActualAttackRange);
						GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Bersaglio fuori raggio!"));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
					}
				}
				else
				{
					// Cliccato a vuoto o su se stesso
					UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Attacco Annullato."));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;

					if (ActionWidgetInstance)
					{
						ActionWidgetInstance->UpdateUI(SelectedUnit);
						ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
					}
				}

				return;
			}

			// CASO B: SELEZIONE NORMALE UNITA' / TOGGLE
			ABaseUnit* ClickedUnit = Cast<ABaseUnit>(ClickedActor);
			if (ClickedUnit)
			{
				if (ClickedUnit->PlayerOwner == 0)
				{
					if (SelectedUnit == ClickedUnit)
					{
						//click di nuovo sulla stessa unità la deseleziona
						SelectedUnit = nullptr;
						GameMode->ClearTileHighlights();
						if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
						CurrentActionState = EPlayerActionState::Idle;
					}
					else
					{
						//seleziona l'unità (anche se ha già attacato, x poter premere FINE TURNO)
						SelectedUnit = ClickedUnit;
						GameMode->ClearTileHighlights();

						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y))
							{
								Tile->SetTileHighlight(true, FLinearColor(1.0f, 1.0f, 0.0f, 1.0f));
								break;
							}
						}

						//Colore x range
						int32 ActualRange = SelectedUnit->MovementRange > 0 ? SelectedUnit->MovementRange : (SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 4 : 6);
						TArray<FVector2D> ValidCells = GameMode->GetReachableCells(SelectedUnit->CurrentGridPosition, ActualRange);

						for (FVector2D CellPos : ValidCells)
						{
							for (ATile* Tile : GameMode->GField->TileArray)
							{
								if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(CellPos.X) &&
									FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(CellPos.Y))
								{
									Tile->SetTileHighlight(true, FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
									break;
								}
							}
						}

						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
					}
				}
			}
			else
			{
				// Click nel vuoto: Deseleziona tutto
				SelectedUnit = nullptr;
				GameMode->ClearTileHighlights();
				if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}
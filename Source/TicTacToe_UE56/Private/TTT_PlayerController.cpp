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

void ATTT_PlayerController::OnLeftMouseClick()
{
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
			// CASO A: STIAMO SELEZIONANDO DOVE MUOVERCI
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

					// PREVIENE MOVIMENTI MULTIPLI (La variabile definita nell'Header!)
					if (UnitThatMovedThisTurn == SelectedUnit)
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
						// 1. Libera la vecchia cella
						for (ATile* Tile : GameMode->GField->TileArray)
						{
							if (Tile &&
								FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) &&
								FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y))
							{
								Tile->SetTileStatus(-1, ETileStatus::EMPTY);
								break;
							}
						}

						// 2. Sposta la pedina FORZANDO il teletrasporto
						FVector NewLocation = ClickedTile->GetActorLocation();
						NewLocation.Z += 60.f;
						SelectedUnit->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

						// 3. Aggiorna logica e traccia il movimento
						SelectedUnit->CurrentGridPosition = TargetGridPos;
						UnitThatMovedThisTurn = SelectedUnit; // Registriamo che si è mossa!

						// NON impostiamo bHasActedThisTurn = true qui, perché l'utente potrebbe voler attaccare dopo essersi mosso!
						ClickedTile->SetTileStatus(SelectedUnit->PlayerOwner, ETileStatus::OCCUPIED);

						// 4. Ripulisci grafica
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						UE_LOG(LogTemp, Warning, TEXT("[Movimento] SPOSTAMENTO SUCCESSO su Cella (%.0f, %.0f)!"), TargetGridPos.X, TargetGridPos.Y);

						// 5. RIAPRIAMO IL MENU per permettere all'utente di Attaccare o Finire il turno
						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] Click su cella non valida o irragiungibile."));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[Movimento] Mossa annullata per click fuori dalla griglia."));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;
				}

				return;
			}

			// CASO C: STIAMO SELEZIONANDO CHI ATTACCARE
			if (CurrentActionState == EPlayerActionState::SelectingAttack && SelectedUnit)
			{
				ABaseUnit* TargetUnit = Cast<ABaseUnit>(ClickedActor);

				if (TargetUnit && TargetUnit->PlayerOwner != SelectedUnit->PlayerOwner) // domanda: se e' nemico oppure no
				{
					// Calcola distanza (Manhattan Distance per griglie a croce)
					int32 DistX = FMath::Abs(FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) - FMath::RoundToInt(TargetUnit->CurrentGridPosition.X));
					int32 DistY = FMath::Abs(FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y) - FMath::RoundToInt(TargetUnit->CurrentGridPosition.Y));
					int32 Distance = DistX + DistY;

					// Recupera il range d'attacco corretto
					int32 ActualAttackRange = SelectedUnit->AttackRange > 0 ? SelectedUnit->AttackRange : (SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);

					if (Distance <= ActualAttackRange)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] BERSAGLIO COLPITO! Distanza: %d. Range: %d"), Distance, ActualAttackRange);

						// Salviamo la posizione in caso il bersaglio venga distrutto
						FVector2D TargetGridPos = TargetUnit->CurrentGridPosition;

						// 1. DANNO AL BERSAGLIO (usiamo le funzioni del tuo BaseUnit)
						int32 Dmg = SelectedUnit->CalculateDamage();
						TargetUnit->TakeDamageAmount(Dmg);

						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Il nemico subisce %d danni. Salute rimasta: %d"), Dmg, TargetUnit->HealthPoints);

						// 2. CONTROLLO MORTE NEMICO
						if (TargetUnit->HealthPoints <= 0)
						{
							UE_LOG(LogTemp, Error, TEXT("[Combattimento] NEMICO DISTRUTTO!"));

							// Libera la cella
							for (ATile* Tile : GameMode->GField->TileArray)
							{
								if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(TargetGridPos.X) &&
									FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(TargetGridPos.Y))
								{
									Tile->SetTileStatus(-1, ETileStatus::EMPTY);
									break;
								}
							}

							GameMode->Player1Units.Remove(TargetUnit);
							GameMode->Player0Units.Remove(TargetUnit);
							//La distruzione dell'Actor la gestisce già il tuo metodo Die() in BaseUnit.cpp
						}
						else
						{
							// 3. IL CONTRATTACCO (Regola d'oro del Prof. Cicala)
							int32 TargetAttackRange = TargetUnit->AttackRange > 0 ? TargetUnit->AttackRange : (TargetUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);

							if (Distance <= TargetAttackRange)
							{
								UE_LOG(LogTemp, Warning, TEXT("[Combattimento] CONTRATTACCO INNESCATO! Il nemico risponde al fuoco!"));
								int32 CounterDmg = TargetUnit->CalculateDamage();

								// Salviamo la posizione in caso moriamo noi
								FVector2D MyGridPos = SelectedUnit->CurrentGridPosition;
								SelectedUnit->TakeDamageAmount(CounterDmg);

								UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Hai subito %d danni. Salute rimasta: %d"), CounterDmg, SelectedUnit->HealthPoints);

								// Se noi moriamo per il contrattacco
								if (SelectedUnit->HealthPoints <= 0)
								{
									UE_LOG(LogTemp, Error, TEXT("[Combattimento] LA TUA UNITA' E' MORTA PER IL CONTRATTACCO!"));

									for (ATile* Tile : GameMode->GField->TileArray)
									{
										if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(MyGridPos.X) &&
											FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(MyGridPos.Y))
										{
											Tile->SetTileStatus(-1, ETileStatus::EMPTY);
											break;
										}
									}
									GameMode->Player0Units.Remove(SelectedUnit);
									GameMode->Player1Units.Remove(SelectedUnit);
									SelectedUnit = nullptr;
								}
							}
						}

						// Pulizia e Fine Turno Automatico dopo aver attaccato
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
						UnitThatMovedThisTurn = nullptr; // Reset del movimento per il prossimo turno
						if (SelectedUnit) SelectedUnit->bHasActedThisTurn = true; // Fine dell'azione per questa pedina
						if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);

						// Se hai attaccato, il turno dell'unità finisce. Passiamo al nemico.
						GameMode->TurnNextPlayer();
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Bersaglio FUORI RAGGIO! Distanza: %d. Il tuo Range: %d"), Distance, ActualAttackRange);
						GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Bersaglio fuori raggio!"));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
					}
				}
				else
				{
					// Cliccato a vuoto o su se stesso
					UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Attacco Annullato (Cliccato a vuoto)."));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;
				}

				return;
			}

			// CASO B: SELEZIONE NORMALE UNITA'
			ABaseUnit* ClickedUnit = Cast<ABaseUnit>(ClickedActor);
			if (ClickedUnit)
			{
				// Puoi selezionare solo se è il tuo turno e l'unità ti appartiene e non ha già agito in modo definitivo
				if (ClickedUnit->PlayerOwner == 0 && !ClickedUnit->bHasActedThisTurn)
				{
					SelectedUnit = ClickedUnit;
					if (ActionWidgetInstance)
					{
						ActionWidgetInstance->UpdateUI(SelectedUnit);
						ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
					}
				}
			}
			else
			{
				SelectedUnit = nullptr;
				if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}
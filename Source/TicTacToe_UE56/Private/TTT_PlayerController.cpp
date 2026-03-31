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
			// ==========================================
			// CASO A: STIAMO SELEZIONANDO DOVE MUOVERCI
			// ==========================================
			if (CurrentActionState == EPlayerActionState::SelectingMove && SelectedUnit)
			{
				if (ClickedActor && ClickedActor->IsA(ATile::StaticClass()))
				{
					ATile* ClickedTile = Cast<ATile>(ClickedActor);
					FVector2D TargetGridPos = ClickedTile->GetGridPosition();

					// FIX per Blueprint Bug
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

					// PREVIENE MOVIMENTI MULTIPLI
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

						// 3. Aggiorna logica
						SelectedUnit->CurrentGridPosition = TargetGridPos;
						UnitThatMovedThisTurn = SelectedUnit; // Registriamo che si è mossa!

						ClickedTile->SetTileStatus(SelectedUnit->PlayerOwner, ETileStatus::OCCUPIED);

						// 4. Ripulisci grafica
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;

						UE_LOG(LogTemp, Warning, TEXT("[Movimento] SPOSTAMENTO SUCCESSO su Cella (%.0f, %.0f)!"), TargetGridPos.X, TargetGridPos.Y);

						// 5. RIAPRIAMO IL MENU
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

				if (TargetUnit && TargetUnit->PlayerOwner != SelectedUnit->PlayerOwner) // È un nemico?
				{
					// --- REQUISITO ELEVAZIONE ---
					// Si può attaccare solo se il nemico è allo stesso livello o inferiore
					float MyZ = SelectedUnit->GetActorLocation().Z;
					float TargetZ = TargetUnit->GetActorLocation().Z;

					// Aggiungiamo un margine di tolleranza di 10.f per evitare problemi di collisioni imperfette
					if (TargetZ > MyZ + 10.f)
					{
						UE_LOG(LogTemp, Error, TEXT("[Combattimento] ATTACCO FALLITO! Il bersaglio e' troppo in alto (Z Nemico: %.1f > Mia Z: %.1f)"), TargetZ, MyZ);
						GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("Attacco Fallito: Il bersaglio e' in una posizione sopraelevata!"));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
						return; // Blocchiamo l'attacco
					}

					// Calcola Distanza di Manhattan
					int32 DistX = FMath::Abs(FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) - FMath::RoundToInt(TargetUnit->CurrentGridPosition.X));
					int32 DistY = FMath::Abs(FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y) - FMath::RoundToInt(TargetUnit->CurrentGridPosition.Y));
					int32 Distance = DistX + DistY;

					int32 ActualAttackRange = SelectedUnit->AttackRange > 0 ? SelectedUnit->AttackRange : (SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);

					if (Distance <= ActualAttackRange)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] BERSAGLIO COLPITO! Distanza: %d. Range: %d"), Distance, ActualAttackRange);

						FVector2D TargetGridPos = TargetUnit->CurrentGridPosition;

						// 1. DANNO AL BERSAGLIO
						int32 Dmg = SelectedUnit->CalculateDamage();
						TargetUnit->TakeDamageAmount(Dmg);

						UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Il nemico subisce %d danni. Salute rimasta: %d"), Dmg, TargetUnit->HealthPoints);

						// 2. CONTROLLO MORTE NEMICO
						if (TargetUnit->HealthPoints <= 0)
						{
							UE_LOG(LogTemp, Error, TEXT("[Combattimento] NEMICO DISTRUTTO!"));

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
						}
						else
						{
							// 3. IL CONTRATTACCO ONLY SNIPER
							// Controlliamo le classi dei due lottatori
							bool bIsAttackerSniper = SelectedUnit->GetClass()->GetName().Contains(TEXT("Sniper"));
							bool bIsTargetSniper = TargetUnit->GetClass()->GetName().Contains(TEXT("Sniper"));
							bool bIsTargetBrawler = TargetUnit->GetClass()->GetName().Contains(TEXT("Brawler"));

							//Innesca SOLO se l'attaccante è uno Sniper
							if (bIsAttackerSniper)
							{
								// per Sniper or un Brawler a distanza 1
								if (bIsTargetSniper || (bIsTargetBrawler && Distance == 1))
								{
									UE_LOG(LogTemp, Warning, TEXT("[Combattimento] REGOLE CICALA: CONTRATTACCO INNESCATO SULLO SNIPER!"));

									// Danno fisso randomico tra 1 e 3
									int32 CounterDmg = FMath::RandRange(1, 3);

									FVector2D MyGridPos = SelectedUnit->CurrentGridPosition;
									SelectedUnit->TakeDamageAmount(CounterDmg);

									UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Il tuo Sniper subisce %d danni da contrattacco. Salute rimasta: %d"), CounterDmg, SelectedUnit->HealthPoints);

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
						}

						// Pulizia e Fine Turno Automatico dopo aver attaccato
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
						UnitThatMovedThisTurn = nullptr;
						if (SelectedUnit) SelectedUnit->bHasActedThisTurn = true;
						if (ActionWidgetInstance) ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);

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
					UE_LOG(LogTemp, Warning, TEXT("[Combattimento] Attacco Annullato."));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;
				}

				return;
			}

			// CASO B: SELEZIONE NORMALE UNITA'
			ABaseUnit* ClickedUnit = Cast<ABaseUnit>(ClickedActor);
			if (ClickedUnit)
			{
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
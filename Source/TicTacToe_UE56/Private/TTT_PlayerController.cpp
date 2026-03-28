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
				FVector TileLocation = ClickedActor->GetActorLocation();
				FVector RelativeLocation = TileLocation - GameMode->GField->GetActorLocation();
				GameMode->SetCellSign(0, RelativeLocation);
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

					// Se l'utente clicca sulla cella in cui è già presente l'unità, annulla lo stato di movimento
					if (FMath::RoundToInt(TargetGridPos.X) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.X) && 
						FMath::RoundToInt(TargetGridPos.Y) == FMath::RoundToInt(SelectedUnit->CurrentGridPosition.Y))
					{
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] Mossa annullata: hai cliccato sulla tua stessa cella."));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
						
						// Riapre il menu normalmente
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

						// 2. Sposta la pedina FORZANDO il teletrasporto (Bypassa le collisioni che potrebbero bloccarlo)
						FVector NewLocation = ClickedTile->GetActorLocation();
						NewLocation.Z += 60.f; 
						SelectedUnit->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
						
						// 3. Aggiorna logica
						SelectedUnit->CurrentGridPosition = TargetGridPos;
						SelectedUnit->bHasActedThisTurn = true;
						ClickedTile->SetTileStatus(SelectedUnit->PlayerOwner, ETileStatus::OCCUPIED);

						// 4. Ripulisci grafica
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
						
						// LOGS DEFINITIVI DI SUCCESSO
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] SPOSTAMENTO SUCCESSO su Cella (%.0f, %.0f)!"), TargetGridPos.X, TargetGridPos.Y);
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Mossa Completata con Successo!"));

						// 5. Invece di deselezionare, RIAPRIAMO IL MENU per permettere all'utente di Attaccare o Finire il turno
						if (ActionWidgetInstance)
						{
							ActionWidgetInstance->UpdateUI(SelectedUnit);
							ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
						}
					}
					else
					{
						GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Mossa non valida: Cella troppo lontana o ostacolo!"));
						UE_LOG(LogTemp, Warning, TEXT("[Movimento] Click su cella non valida o irragiungibile."));
						GameMode->ClearTileHighlights();
						CurrentActionState = EPlayerActionState::Idle;
					}
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Mossa Annullata (Hai cliccato fuori)!"));
					UE_LOG(LogTemp, Warning, TEXT("[Movimento] Mossa annullata per click fuori dalla griglia."));
					GameMode->ClearTileHighlights();
					CurrentActionState = EPlayerActionState::Idle;
				}
				
				return; 
			}

			// CASO B: SELEZIONE NORMALE UNITA'
			ABaseUnit* ClickedUnit = Cast<ABaseUnit>(ClickedActor);
			if (ClickedUnit)
			{
				if (ClickedUnit->PlayerOwner == 0)
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
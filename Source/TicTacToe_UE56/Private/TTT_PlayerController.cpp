// Fill out your copyright notice in the Description page of Project Settings.

#include "TTT_PlayerController.h"
#include "TTT_GameMode.h"
#include "Blueprint/UserWidget.h"
#include "GameField.h" // Serve per capire se stiamo cliccando la griglia

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

	// Creazione della UI (Menu Azioni)
	if (ActionWidgetClass)
	{
		ActionWidgetInstance = CreateWidget<UUnitActionWidget>(this, ActionWidgetClass);
		if (ActionWidgetInstance)
		{
			// Aggiungiamo alla viewport ma lo rendiamo invisibile finché non selezioniamo un'unità
			ActionWidgetInstance->AddToViewport();
			ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			UE_LOG(LogTemp, Warning, TEXT("UI Menu Inizializzato con successo."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ERRORE CRITICO: Non hai assegnato ActionWidgetClass nel BP_PlayerController!"));
	}
}

void ATTT_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// FIX: Bypassa le impostazioni di progetto di UE 5.6 e collega direttamente l'hardware del mouse!
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATTT_PlayerController::OnLeftMouseClick);
}

void ATTT_PlayerController::OnLeftMouseClick()
{
	// LOG per capire se il click viene percepito dal motore
	UE_LOG(LogTemp, Warning, TEXT("[Click] Mouse cliccato!"));

	ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode) return;

	// Se non è il nostro turno, ignoriamo il click
	if (GameMode->CurrentPlayer != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Click] Rifiutato: Non e' il tuo turno. Tocca al giocatore %d"), GameMode->CurrentPlayer);
		return;
	}

	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		AActor* ClickedActor = HitResult.GetActor();

		if (ClickedActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Click] Hai colpito l'Actor: %s"), *ClickedActor->GetName());
		}

		// FASE I: PIAZZAMENTO (click la Griglia)
		if (GameMode->CurrentGameState == EGameState::Placement)
		{
			// Se stiamo cliccando su una cella (ATile)
			if (ClickedActor && ClickedActor->IsA(ATile::StaticClass()))
			{
				// FIX MATEMATICO: Invece del punto esatto del mouse, prendiamo il centro perfetto della cella
				FVector TileLocation = ClickedActor->GetActorLocation();

				// Ricalcoliamo la posizione relativa al centro della scacchiera
				FVector RelativeLocation = TileLocation - GameMode->GField->GetActorLocation();

				UE_LOG(LogTemp, Warning, TEXT("[Piazzamento] Cella valida cliccata. Invio mossa..."));

				// Chiamiamo il GameMode per fargli processare la nostra mossa (0 = Umano)
				GameMode->SetCellSign(0, RelativeLocation);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Piazzamento] Errore: Non hai cliccato una cella calpestabile!"));
			}
		}
		// FASE II: GIOCO (Selezione Unità e UI)
		else if (GameMode->CurrentGameState == EGameState::Playing)
		{
			ABaseUnit* ClickedUnit = Cast<ABaseUnit>(ClickedActor);

			if (ClickedUnit)
			{
				if (ClickedUnit->PlayerOwner == 0) // L'unità è nostra
				{
					SelectedUnit = ClickedUnit;
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("Unita' Selezionata!"));
					UE_LOG(LogTemp, Warning, TEXT("[Selezione] Hai selezionato la tua unita'. Apro il Menu..."));

					// Aggiorniamo e mostriamo il Widget Menu che hai creato!
					if (ActionWidgetInstance)
					{
						ActionWidgetInstance->UpdateUI(SelectedUnit);
						ActionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
					}
				}
				else // L'unità è nemica
				{
					UE_LOG(LogTemp, Warning, TEXT("[Selezione] Unita' nemica!"));
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Non puoi comandare un'unita' nemica!"));
				}
			}
			else
			{
				// Se clicchiamo sul vuoto (o su una cella invece che sull'unità) -> menu va in OFF
				SelectedUnit = nullptr;
				if (ActionWidgetInstance)
				{
					ActionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Click] Hai cliccato nel vuoto cosmico."));
	}
}
// Fill out your copyright notice in the Description page of Project Settings.

#include "UnitActionWidget.h"
#include "TTT_PlayerController.h"
#include "TTT_GameMode.h"
#include "Tile.h" // Necessario per chiamare SetTileHighlight sulle celle

void UUnitActionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MoveButton)
	{
		MoveButton->OnClicked.AddDynamic(this, &UUnitActionWidget::OnMoveButtonClicked);
	}
	if (AttackButton)
	{
		AttackButton->OnClicked.AddDynamic(this, &UUnitActionWidget::OnAttackButtonClicked);
	}
	if (EndTurnButton)
	{
		EndTurnButton->OnClicked.AddDynamic(this, &UUnitActionWidget::OnEndTurnButtonClicked);
	}
}

void UUnitActionWidget::UpdateUI(ABaseUnit* InSelectedUnit)
{
	if (!InSelectedUnit) return;

	CurrentUnit = InSelectedUnit;

	FString NameStr = (CurrentUnit->AttackRange > 1) ? TEXT("SNIPER") : TEXT("BRAWLER");
	if (UnitNameText) UnitNameText->SetText(FText::FromString(NameStr));

	if (HPText)
	{
		FString HPStr = FString::Printf(TEXT("HP: %d / %d"), CurrentUnit->HealthPoints, CurrentUnit->MaxHealthPoints);
		HPText->SetText(FText::FromString(HPStr));
	}

	if (DamageText)
	{
		FString DmgStr = FString::Printf(TEXT("Danno: %d - %d"), CurrentUnit->MinDamage, CurrentUnit->MaxDamage);
		DamageText->SetText(FText::FromString(DmgStr));
	}

	if (MovementText)
	{
		FString MovStr = FString::Printf(TEXT("Movimento: %d"), CurrentUnit->MovementRange);
		MovementText->SetText(FText::FromString(MovStr));
	}

	bool bCanAct = !CurrentUnit->bHasActedThisTurn;
	if (MoveButton) MoveButton->SetIsEnabled(bCanAct);
	if (AttackButton) AttackButton->SetIsEnabled(bCanAct);
}

void UUnitActionWidget::OnMoveButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[UI] Hai premuto il tasto MUOVI!"));

	ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetOwningPlayer());
	if (PC && CurrentUnit)
	{
		PC->CurrentActionState = EPlayerActionState::SelectingMove;

		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode && GameMode->GField)
		{
			// Calcola le celle raggiungibili
			TArray<FVector2D> ValidCells = GameMode->GetReachableCells(CurrentUnit->CurrentGridPosition, CurrentUnit->MovementRange);

			for (FVector2D CellPos : ValidCells)
			{
				// FIX: Accesso diretto e sicuro alla TileMap al posto del vecchio GetTileAtPosition
				if (GameMode->GField->TileMap.Contains(CellPos))
				{
					ATile* Tile = GameMode->GField->TileMap[CellPos];
					if (Tile)
					{
						// Colore Verde (R=0, G=1, B=0)
						Tile->SetTileHighlight(true, FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
					}
				}
			}
		}
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Seleziona una cella per muoverti!"));
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UUnitActionWidget::OnAttackButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[UI] Hai premuto il tasto ATTACCA!"));
}

void UUnitActionWidget::OnEndTurnButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[UI] Hai premuto il tasto FINE TURNO!"));

	// Nascondiamo noi stessi
	SetVisibility(ESlateVisibility::Hidden);

	// Troviamo il GameMode e passiamo il turno
	if (GetWorld())
	{
		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			GameMode->TurnNextPlayer();
		}
	}
}
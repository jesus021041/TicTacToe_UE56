// Fill out your copyright notice in the Description page of Project Settings.

#include "UnitActionWidget.h"
#include "TTT_PlayerController.h"
#include "TTT_GameMode.h"
#include "Tile.h"

void UUnitActionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MoveButton) MoveButton->OnClicked.AddDynamic(this, &UUnitActionWidget::OnMoveButtonClicked);
	if (AttackButton) AttackButton->OnClicked.AddDynamic(this, &UUnitActionWidget::OnAttackButtonClicked);
	if (EndTurnButton) EndTurnButton->OnClicked.AddDynamic(this, &UUnitActionWidget::OnEndTurnButtonClicked);
}

void UUnitActionWidget::UpdateUI(ABaseUnit* InSelectedUnit)
{
	if (!InSelectedUnit) return;
	CurrentUnit = InSelectedUnit;

	FString NameStr = (CurrentUnit->GetClass()->GetName().Contains(TEXT("Sniper"))) ? TEXT("SNIPER") : TEXT("BRAWLER");
	if (UnitNameText) UnitNameText->SetText(FText::FromString(NameStr));

	if (HPText)
	{
		FString HPStr = FString::Printf(TEXT("HP: %d / %d"), CurrentUnit->HealthPoints, CurrentUnit->MaxHP);
		HPText->SetText(FText::FromString(HPStr));
	}

	if (DamageText)
	{
		FString DmgStr = FString::Printf(TEXT("Danno: %d - %d"), CurrentUnit->MinDamage, CurrentUnit->MaxDamage);
		DamageText->SetText(FText::FromString(DmgStr));
	}

	if (MovementText)
	{
		int32 ActualRange = CurrentUnit->MovementRange;
		if (ActualRange <= 0) ActualRange = CurrentUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 4 : 6;
		FString MovStr = FString::Printf(TEXT("Movimento: %d"), ActualRange);
		MovementText->SetText(FText::FromString(MovStr));
	}

	bool bCanAct = !CurrentUnit->bHasActedThisTurn;

	// EXTRA SICUREZZA: Disabilita il tasto Muovi se si è già mossa in questo turno
	ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetOwningPlayer());
	if (PC && PC->UnitThatMovedThisTurn == CurrentUnit)
	{
		if (MoveButton) MoveButton->SetIsEnabled(false);
	}
	else
	{
		if (MoveButton) MoveButton->SetIsEnabled(bCanAct);
	}

	if (AttackButton) AttackButton->SetIsEnabled(bCanAct);
}

void UUnitActionWidget::OnMoveButtonClicked()
{
	ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetOwningPlayer());
	if (PC && CurrentUnit)
	{
		PC->CurrentActionState = EPlayerActionState::SelectingMove;

		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode && GameMode->GField)
		{
			int32 ActualRange = CurrentUnit->MovementRange;
			if (ActualRange <= 0) ActualRange = CurrentUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 4 : 6;

			TArray<FVector2D> ValidCells = GameMode->GetReachableCells(CurrentUnit->CurrentGridPosition, ActualRange);

			//ILLUMINAZIONE VERDE (Movimento) -> TODO  [nn si vede]
			for (FVector2D CellPos : ValidCells)
			{
				for (ATile* Tile : GameMode->GField->TileArray)
				{
					if (Tile && FMath::RoundToInt(Tile->GetGridPosition().X) == FMath::RoundToInt(CellPos.X) &&
						FMath::RoundToInt(Tile->GetGridPosition().Y) == FMath::RoundToInt(CellPos.Y))
					{
						Tile->SetTileHighlight(true, FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)); // Verde
						break;
					}
				}
			}
		}
	}
	SetVisibility(ESlateVisibility::Hidden);
}

void UUnitActionWidget::OnAttackButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("[UI] Hai premuto il tasto ATTACCA!"));

	ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetOwningPlayer());
	if (PC && CurrentUnit)
	{
		// COMUNICHIAMO AL CONTROLLER DI PREPARARSI ALL'ATTACCO
		PC->CurrentActionState = EPlayerActionState::SelectingAttack;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("Seleziona un nemico nelle caselle ROSSE!"));

		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode && GameMode->GField)
		{
			// Calcoliamo il range di attacco
			int32 ActualAttackRange = CurrentUnit->AttackRange > 0 ? CurrentUnit->AttackRange : (CurrentUnit->GetClass()->GetName().Contains(TEXT("Sniper")) ? 10 : 1);

			//ILLUMINAZIONE ROSSA (Attacco) -> TODO [nn si vede]
			for (ATile* Tile : GameMode->GField->TileArray)
			{
				if (Tile)
				{
					int32 DistX = FMath::Abs(FMath::RoundToInt(CurrentUnit->CurrentGridPosition.X) - FMath::RoundToInt(Tile->GetGridPosition().X));
					int32 DistY = FMath::Abs(FMath::RoundToInt(CurrentUnit->CurrentGridPosition.Y) - FMath::RoundToInt(Tile->GetGridPosition().Y));

					// La Distanza di Manhattan. Se rientra nel Range (ma > 0 per non colpire se stessi)
					if ((DistX + DistY) > 0 && (DistX + DistY) <= ActualAttackRange)
					{
						Tile->SetTileHighlight(true, FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
					}
				}
			}
		}
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UUnitActionWidget::OnEndTurnButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
	if (GetWorld())
	{
		ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
		ATTT_PlayerController* PC = Cast<ATTT_PlayerController>(GetOwningPlayer());
		if (GameMode && PC)
		{
			if (CurrentUnit) CurrentUnit->bHasActedThisTurn = true;

			PC->CurrentActionState = EPlayerActionState::Idle;
			PC->UnitThatMovedThisTurn = nullptr;
			GameMode->ClearTileHighlights();

			GameMode->TurnNextPlayer();
		}
	}
}
// Fill out your copyright notice in the Description page of Project Settings.

#include "UnitActionWidget.h"

void UUnitActionWidget::UpdateUI(ABaseUnit* SelectedUnit)
{
	if (!SelectedUnit) return;

	// Capiamo se è uno Sniper o un Brawler dal suo Range di Attacco
	FString NameStr = (SelectedUnit->AttackRange > 1) ? TEXT("SNIPER") : TEXT("BRAWLER");
	if (UnitNameText)
	{
		UnitNameText->SetText(FText::FromString(NameStr));
	}

	if (HPText)
	{
		FString HPStr = FString::Printf(TEXT("HP: %d / %d"), SelectedUnit->HealthPoints, SelectedUnit->MaxHealthPoints);
		HPText->SetText(FText::FromString(HPStr));
	}

	if (DamageText)
	{
		FString DmgStr = FString::Printf(TEXT("Danno: %d - %d"), SelectedUnit->MinDamage, SelectedUnit->MaxDamage);
		DamageText->SetText(FText::FromString(DmgStr));
	}

	if (MovementText)
	{
		FString MovStr = FString::Printf(TEXT("Movimento: %d"), SelectedUnit->MovementRange);
		MovementText->SetText(FText::FromString(MovStr));
	}

	bool bCanAct = !SelectedUnit->bHasActedThisTurn;

	if (MoveButton) MoveButton->SetIsEnabled(bCanAct);
	if (AttackButton) AttackButton->SetIsEnabled(bCanAct);
}
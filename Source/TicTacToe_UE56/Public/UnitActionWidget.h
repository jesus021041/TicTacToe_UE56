// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseUnit.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "UnitActionWidget.generated.h"

UCLASS()
class TICTACTOE_UE56_API UUnitActionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Funzione per aggiornare i testi quando clicchi su un'unità
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateUI(ABaseUnit* InSelectedUnit);

protected:
	// Costruttore nativo della UI (viene chiamato quando il widget viene creato)
	virtual void NativeConstruct() override;

	// --- FUNZIONI LEGATE AI BOTTONI ---
	UFUNCTION()
	void OnMoveButtonClicked();

	UFUNCTION()
	void OnAttackButtonClicked();

	UFUNCTION()
	void OnEndTurnButtonClicked();

	// Riferimento interno all'unità attualmente mostrata
	UPROPERTY()
	ABaseUnit* CurrentUnit;

	//COMPONENTI UI

	UPROPERTY(meta = (BindWidget))
	UTextBlock* UnitNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HPText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MovementText;

	UPROPERTY(meta = (BindWidget))
	UButton* MoveButton;

	UPROPERTY(meta = (BindWidget))
	UButton* AttackButton;

	UPROPERTY(meta = (BindWidget))
	UButton* EndTurnButton;
};
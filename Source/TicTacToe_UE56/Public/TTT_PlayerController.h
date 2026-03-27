// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaseUnit.h"
#include "UnitActionWidget.h" // Includiamo il nostro nuovo Widget
#include "TTT_PlayerController.generated.h"

UCLASS()
class TICTACTOE_UE56_API ATTT_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATTT_PlayerController();

	// Puntatore all'unità attualmente selezionata
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
	ABaseUnit* SelectedUnit;

	// La classe del Blueprint che sceglieremo nell'editor
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUnitActionWidget> ActionWidgetClass;

	// L'istanza vera e propria creata a schermo
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UUnitActionWidget* ActionWidgetInstance;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// Funzione chiamata al click sinistro del mouse
	void OnLeftMouseClick();
};
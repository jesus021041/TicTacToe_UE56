// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaseUnit.h"
#include "UnitActionWidget.h"
#include "TTT_PlayerController.generated.h"

// Enum per capire cosa sta facendo l'utente con il mouse in questo momento
UENUM(BlueprintType)
enum class EPlayerActionState : uint8
{
	Idle			UMETA(DisplayName = "Fermo"),
	SelectingMove	UMETA(DisplayName = "Selezionando Movimento"),
	SelectingAttack	UMETA(DisplayName = "Selezionando Attacco")
};

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

	// Stato attuale dell'azione del giocatore
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EPlayerActionState CurrentActionState = EPlayerActionState::Idle;

	// TRACCIA L'UNITA' CHE SI E' MOSSA QUESTO TURNO
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ABaseUnit* UnitThatMovedThisTurn = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// Funzione chiamata al click sinistro del mouse
	void OnLeftMouseClick();
};
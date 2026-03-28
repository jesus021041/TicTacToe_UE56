// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameField.h"
#include "BaseUnit.h"
#include "TTT_ConfigData.h" // Aggiunto l'include corretto per la configurazione
#include "TTT_GameInstance.h"
#include "TTT_PlayerInterface.h" // FIX: Aggiunto l'include mancante per l'interfaccia!
#include "TTT_GameMode.generated.h"

// Enumeratore per gli stati di gioco
UENUM(BlueprintType)
enum class EGameState : uint8
{
	CoinFlip		UMETA(DisplayName = "Lancio Moneta"),
	Placement		UMETA(DisplayName = "Piazzamento"),
	Playing			UMETA(DisplayName = "In Gioco"),
	GameOver		UMETA(DisplayName = "Fine Partita")
};

UCLASS()
class TICTACTOE_UE56_API ATTT_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTT_GameMode();

	// Variabili di classe
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<AGameField> GameFieldClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
	AGameField* GField;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<ABaseUnit> SniperClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<ABaseUnit> BrawlerClass;

	// Riferimenti alle unità in campo
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Units")
	TArray<ABaseUnit*> Player0Units;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Units")
	TArray<ABaseUnit*> Player1Units;

	//LOGICA MOVE
	UFUNCTION(BlueprintCallable, Category = "Movement")
	TArray<FVector2D> GetReachableCells(FVector2D StartGridPos, int32 MovementRange);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ClearTileHighlights();

	UFUNCTION(BlueprintCallable, Category = "Grid")
	ABaseUnit* GetUnitAtGridPosition(FVector2D GridPos);

	UPROPERTY(EditDefaultsOnly)
	UTTT_ConfigData* GridData;

	int32 FieldSize;
	float TileSize;
	float CellPadding;

	// Stato del gioco
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
	EGameState CurrentGameState;

	int32 Player0UnitsPlaced;
	int32 Player1UnitsPlaced;

	TArray<ITTT_PlayerInterface*> Players;
	int32 StartingPlayer;
	int32 CurrentPlayer;
	int32 MoveCounter;
	bool IsGameOver;
	FTimerHandle ResetTimerHandle;

	// Funzioni di base
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void PerformCoinFlip();
	void SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition);
	int32 GetNextPlayer(int32 Player);
	void TurnNextPlayer();
};
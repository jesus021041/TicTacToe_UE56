// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameField.h"
#include "BaseUnit.h"
#include "TTT_ConfigData.h"
#include "TTT_PlayerInterface.h"
#include "TTT_GameMode.generated.h"

// --- REQUISITO: Macchina a Stati per le Fasi della Partita ---
UENUM(BlueprintType)
enum class EGameState : uint8
{
	CoinFlip,
	Placement,
	Playing,
	GameOver
};

UCLASS()
class TICTACTOE_UE56_API ATTT_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTT_GameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Stato attuale della partita
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EGameState CurrentGameState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool IsGameOver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 MoveCounter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 CurrentPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 StartingPlayer;

	// --- Contatori per il Piazzamento ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 Player0UnitsPlaced;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 Player1UnitsPlaced;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	UTTT_ConfigData* GridData;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TSubclassOf<AGameField> GameFieldClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
	AGameField* GField;

	// --- Classi delle Unità da assegnare nel Blueprint ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Config")
	TSubclassOf<ABaseUnit> SniperClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Config")
	TSubclassOf<ABaseUnit> BrawlerClass;

	// Array dei giocatori (0 = Umano, 1 = AI)
	TArray<ITTT_PlayerInterface*> Players;

	FTimerHandle ResetTimerHandle;

	int32 FieldSize;
	float TileSize;
	float CellPadding;

	// Funzioni principali
	void PerformCoinFlip();
	void TurnNextPlayer();
	int32 GetNextPlayer(int32 Player);

	// Funzione per piazzare (e in futuro muovere) le unità
	void SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition);
};
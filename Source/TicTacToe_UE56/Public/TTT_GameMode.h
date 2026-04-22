// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameField.h"
#include "BaseUnit.h"
#include "TTT_ConfigData.h" 
#include "TTT_GameInstance.h"
#include "TTT_PlayerInterface.h"
#include "Blueprint/UserWidget.h"
#include "ActionLogWidget.h"
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

	//4 CLASSI DELLE UNITA' Lato Player && IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<ABaseUnit> SniperClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<ABaseUnit> BrawlerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<ABaseUnit> SniperAIClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game")
	TSubclassOf<ABaseUnit> BrawlerAIClass;

	// Riferimenti alle unità in campo
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Units")
	TArray<ABaseUnit*> Player0Units;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Units")
	TArray<ABaseUnit*> Player1Units;

	// logica di movimento & HIGHLIGHT
	UFUNCTION(BlueprintCallable, Category = "Movement")
	TArray<FVector2D> GetReachableCells(FVector2D StartGridPos, int32 MovementRange);

	//Calcolare percorso esatto (to-move)
	UFUNCTION(BlueprintCallable, Category = "Movement")
	TArray<FVector2D> FindPath(FVector2D StartGridPos, FVector2D TargetGridPos);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 CurrentPlayer;

	int32 Player0UnitsPlaced;
	int32 Player1UnitsPlaced;

	TArray<ITTT_PlayerInterface*> Players;
	int32 StartingPlayer;
	int32 MoveCounter;
	bool IsGameOver;
	FTimerHandle ResetTimerHandle;

	//GESTIONE TORRI => vari stati
	// Mappa le posizioni delle torri al loro stato: -1 (Neutrale), 0 (P0), 1 (P1), 2 (Contesa)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towers")
	TMap<FVector2D, int32> TowerStates;

	// Timer per la vittoria (devono raggiungere 2 turni consecutivi con 2+ torri)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towers")
	int32 Player0WinTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towers")
	int32 Player1WinTimer;

	UFUNCTION(BlueprintCallable, Category = "Towers")
	void EvaluateTowers();

	UFUNCTION(BlueprintCallable, Category = "Towers")
	void UpdateTowerVisuals();

	//GESTIONE VITTORIA && GAME OVER
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

	UPROPERTY()
	UUserWidget* GameOverWidgetInstance;

	//STORICO DELLE MOSSE -> HUD 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UActionLogWidget> ActionLogWidgetClass;

	UPROPERTY()
	UActionLogWidget* ActionLogWidgetInstance;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void AddToActionLog(const FString& Message, FLinearColor TextColor = FLinearColor::White);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	FString GetCellName(FVector2D GridPos);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	FString GetUnitShortName(ABaseUnit* Unit);

	UFUNCTION(BlueprintCallable, Category = "Game")
	void EndGame(int32 WinnerPlayer);

	// Funzioni di base
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void PerformCoinFlip();
	void SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition);
	int32 GetNextPlayer(int32 Player);
	void TurnNextPlayer();

	//Aggiorna la UI delle torri
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateTowerScoreUI(int32 P0Score, int32 P1Score);

	//Variabili x change pt 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 CurrentP0Score = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
	int32 CurrentP1Score = 0;

	//Visualizzare i msg:
	// Variabile che conterrà il testo da mostrare (letta dal Widget)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	FString TopLeftMessage;

	// Variabile che conterrà il colore del testo (letta dal Widget)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	FLinearColor TopLeftColor;

	// Dichiarazione della funzione che useremo nel .cpp
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowTopLeftMessage(const FString& Msg, FLinearColor Color, float Duration);
};
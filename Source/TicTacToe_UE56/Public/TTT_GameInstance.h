// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TTT_GameInstance.generated.h"

class APawn;
/**
 * */
UCLASS()
class TICTACTOE_UE56_API UTTT_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	//Confing iniziali -> menu

	//Variabile per la grandezza della griglia
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 CustomGridSize = 25; //da impostazione

	// Classe dell'IA selezionata
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSubclassOf<APawn> SelectedAIClass;

	// score value for human player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 ScoreHumanPlayer = 0;

	// score value for AI player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	int32 ScoreAiPlayer = 0;

	// message to show every turn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
	FString CurrentTurnMessage = "Current Player";

	// increment the score for human player
	void IncrementScoreHumanPlayer();

	// increment the score for AI player
	void IncrementScoreAiPlayer();

	// get the score for human player
	UFUNCTION(BlueprintCallable)
	int32 GetScoreHumanPlayer();

	// get the score for AI player
	UFUNCTION(BlueprintCallable)
	int32 GetScoreAiPlayer();

	// reset scores
	UFUNCTION(BlueprintCallable)
	void ResetScores();

	// get the current turn message
	UFUNCTION(BlueprintCallable)
	FString GetTurnMessage();

	// set the turn message
	UFUNCTION(BlueprintCallable)
	void SetTurnMessage(FString Message);

	// Variabile per il Seed personalizzato 
	//se 0 => Casuale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 CustomSeed = 0;
};
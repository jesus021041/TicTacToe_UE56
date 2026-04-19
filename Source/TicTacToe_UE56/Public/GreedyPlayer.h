// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TTT_PlayerInterface.h"
#include "TTT_GameInstance.h"
#include "BaseUnit.h"
#include "GreedyPlayer.generated.h"

UCLASS()
class TICTACTOE_UE56_API AGreedyPlayer : public APawn, public ITTT_PlayerInterface
{
	GENERATED_BODY()

public:
	AGreedyPlayer();

	UPROPERTY()
	UTTT_GameInstance* GameInstance;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void OnTurn() override;
	virtual void OnWin() override;
	virtual void OnLose() override;

private:
	FTimerHandle AI_TurnTimerHandle;
	bool bIsMovingUnit = false;

	UPROPERTY()
	ABaseUnit* CurrentCommandUnit = nullptr;

	void ExecuteAIAttack();

	//ALGORITMO EURISTICO (GREEDY BEST-FIRST)
	TArray<FVector2D> FindGreedyPath(FVector2D Start, FVector2D Target, int32 MaxSteps);
};
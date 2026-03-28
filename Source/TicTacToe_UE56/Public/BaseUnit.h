// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.generated.h"

// Enumeratore per il tipo di attacco
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee       UMETA(DisplayName = "Mischia"),
	Ranged      UMETA(DisplayName = "Distanza")
};

UCLASS(Abstract)
class TICTACTOE_UE56_API ABaseUnit : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaseUnit();

	// Componente visivo (Mesh)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* UnitMesh;

	// === STATISTICHE UNITÀ (Modificabili dai Blueprint figli) ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MaxHealthPoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit Stats")
	int32 HealthPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MinDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MaxDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MovementRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	EAttackType AttackType;

	//GESTIONE GIOCO
	// 0 per Umano && 1 per IA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
	int32 PlayerOwner;

	// La coordinata X,Y sulla scacchiera in cui si trova la pedina
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
	FVector2D CurrentGridPosition;

	// Indica se l'unità ha già fatto la sua mossa questo turno
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game")
	bool bHasActedThisTurn = false;

	// === METODI DI COMBATTIMENTO ===
	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 CalculateDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TakeDamageAmount(int32 DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Die();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};
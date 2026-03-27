// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.generated.h"

// Enum per definire il tipo di attacco come richiesto dal PDF
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Melee       UMETA(DisplayName = "Attacco a corto raggio"),
	Ranged      UMETA(DisplayName = "Attacco a distanza")
};

UCLASS(Abstract) // "Abstract" impedisce di spawnare una BaseUnit generica. Si spawneranno solo Sniper e Brawler.
class TICTACTOE_UE56_API ABaseUnit : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABaseUnit();

	// Variabile che identifica di quante caselle (MAX) può muoversi l'unità
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MovementRange;

	// Tipo di Attacco
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	EAttackType AttackType;

	// Quante caselle sono necessarie per effettuare l'attacco
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 AttackRange;

	// Danno Minimo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MinDamage;

	// Danno Massimo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 MaxDamage;

	// Punti Vita
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
	int32 HealthPoints;

	// Punti Vita Massimi (utile per la UI)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit Stats")
	int32 MaxHealthPoints;

	// Identificativo del giocatore a cui appartiene (0 = Umano, 1 = AI)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
	int32 PlayerOwner;

	// Indica se l'unità ha già agito in questo turno
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
	bool bHasActedThisTurn;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Componente visivo dell'unità (la mesh 3D)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* UnitMesh;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Funzione per calcolare il danno inflitto (estrae un numero random tra Min e Max)
	UFUNCTION(BlueprintCallable, Category = "Unit Actions")
	int32 CalculateDamage() const;

	// Funzione per ricevere danno
	UFUNCTION(BlueprintCallable, Category = "Unit Actions")
	virtual void TakeDamageAmount(int32 DamageAmount);

	// Funzione chiamata quando gli HP scendono a zero
	virtual void Die();
};
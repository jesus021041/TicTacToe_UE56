// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

// Se questo enum è in un altro file nel tuo progetto, puoi rimuovere questa dichiarazione
UENUM(BlueprintType)
enum class ETileStatus : uint8
{
	EMPTY UMETA(DisplayName = "Empty"),
	OCCUPIED UMETA(DisplayName = "Occupied")
};

UCLASS()
class TICTACTOE_UE56_API ATile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Scene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile State")
	ETileStatus Status;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile State")
	int32 PlayerOwner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile State")
	FVector2D TileGridPosition;

	// --- NUOVE VARIABILI PER LA MAPPA 25x25 ---

	// Livello di altezza (0=Acqua, 1=Pianura, 2,3,4=Montagne)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile State")
	int32 ElevationLevel;

	// Se true, le unità non possono camminarci sopra (Acqua o Torri)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile State")
	bool bIsObstacle;

	// Array di materiali da impostare nell'editor di Unreal (0=Blu, 1=Verde, ecc.)
	UPROPERTY(EditDefaultsOnly, Category = "Tile Appearance")
	TArray<class UMaterialInterface*> ElevationMaterials;

	// --- FINE NUOVE VARIABILI ---

	void SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus);
	ETileStatus GetTileStatus();
	int32 GetOwner();

	void SetGridPosition(const double InX, const double InY);
	FVector2D GetGridPosition();

	// Nuova funzione per impostare l'altezza e applicare il colore
	void SetElevationLevel(int32 Level);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
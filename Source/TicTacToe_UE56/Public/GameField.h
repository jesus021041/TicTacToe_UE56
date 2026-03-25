// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tile.h"
#include "GameFramework/Actor.h"
#include "TTT_ConfigData.h"
#include "GameField.generated.h"

// macro declaration for a dynamic multicast delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReset);

UCLASS()
class TICTACTOE_UE56_API AGameField : public AActor
{
	GENERATED_BODY()

public:
	// keeps track of tiles
	UPROPERTY(Transient)
	TArray<ATile*> TileArray;

	//given a position returns a tile
	UPROPERTY(Transient)
	TMap<FVector2D, ATile*> TileMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NextCellPositionMultiplier;

	static const int32 NOT_ASSIGNED = -1;

	// BlueprintAssignable Usable with Multicast Delegates only. Exposes the property for assigning in Blueprints.
	// declare a variable of type FOnReset (delegate)
	UPROPERTY(BlueprintAssignable)
	FOnReset OnResetEvent;

	// size of field
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Size;

	// size of winning line (Mantenuto temporaneamente per non rompere compatibilità incrociate)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 WinSize;

	// TSubclassOf template class that provides UClass type safety
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATile> TileClass;

	// tile padding percentage
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CellPadding;

	// tile size
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TileSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	UTTT_ConfigData* GridData;

	// --- PARAMETRI PER LA GENERAZIONE PROCEDURALE (PERLIN NOISE) ---

	// Seed casuale per avere mappe diverse ad ogni esecuzione
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapConfig")
	int32 RandomSeed;

	// Scala del Perlin Noise (più basso = colline più dolci, più alto = variazioni brusche)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapConfig")
	float NoiseScale;

	// Moltiplicatore visivo per l'altezza Z ad ogni livello (0-4)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapConfig")
	float ZMultiplier;

	// ---------------------------------------------------------------

	// Sets default values for this actor's properties
	AGameField();

	// Called when an instance of this class is placed (in editor) or spawned
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// remove all signs from the field
	UFUNCTION(BlueprintCallable)
	void ResetField();

	// generate an empty game field
	void GenerateField();

	// --- FUNZIONI PER LE TORRI ---

	// Gestisce il posizionamento delle 3 torri sulla mappa
	void SpawnTowers();

	// Trova la cella valida più vicina al punto ideale dove spawnare una torre
	ATile* GetNearestValidTileForTower(FVector2D TargetPos);

	// -----------------------------

	// return a (x,y) position given a hit (click) on a field tile
	FVector2D GetPosition(const FHitResult& Hit);

	// return the array of tile pointers
	TArray<ATile*>& GetTileArray();

	// return a relative position given (x,y) position
	FVector GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const;

	// return (x,y) position given a relative position
	FVector2D GetXYPositionByRelativeLocation(const FVector& Location) const;

	//public:	
	//	// Called every frame
	//	virtual void Tick(float DeltaTime) override;

};
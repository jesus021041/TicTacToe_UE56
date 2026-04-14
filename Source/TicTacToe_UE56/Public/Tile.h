// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

UENUM()
enum class ETileStatus : uint8
{
	EMPTY     UMETA(DisplayName = "Empty"),
	OCCUPIED  UMETA(DisplayName = "Occupied"),
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

	//Mesh dedicata per mostrare i colori dell'highlight
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* HighlightMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETileStatus Status;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PlayerOwner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D TileGridPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
	int32 ElevationLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	bool bIsObstacle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
	TArray<UMaterialInterface*> ElevationMaterials;

	//l'highlight e imposta il colore
	void SetTileHighlight(bool bHighlight, FLinearColor Color);

	void SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus);
	ETileStatus GetTileStatus();
	int32 GetOwner();

	void SetGridPosition(const double InX, const double InY);
	FVector2D GetGridPosition();

	UFUNCTION(BlueprintCallable, Category = "Tile")
	void SetElevationLevel(int32 Level);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
// Fill out your copyright notice in the Description page of Project Settings


#include "BaseSign.h"
#include "GameField.h"
#include "TTT_GameMode.h"
#include "Kismet/GameplayStatics.h" // Aggiunto per poter cercare gli attori nel mondo

// Sets default values
ABaseSign::ABaseSign()
{
	// Disattiviamo il Tick perché per i Segni/Torri non serve e consuma solo performance
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ABaseSign::BeginPlay()
{
	Super::BeginPlay();

	// FIX CRASH: Cerchiamo il GameField direttamente nel mondo in modo sicuro
	// invece di leggere la variabile GField del GameMode che potrebbe non essere ancora inizializzata.
	AGameField* Field = Cast<AGameField>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameField::StaticClass()));

	if (Field)
	{
		// Se lo abbiamo trovato, ci iscriviamo all'evento
		Field->OnResetEvent.AddDynamic(this, &ABaseSign::SelfDestroy);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BaseSign non è riuscito a trovare il GameField nel livello!"));
	}
}

// Called every frame
void ABaseSign::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseSign::SelfDestroy()
{
	Destroy();
}
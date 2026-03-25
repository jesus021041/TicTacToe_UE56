// Fill out your copyright notice in the Description page of Project Settings.


#include "TTT_GameMode.h"
#include "TTT_PlayerController.h"
#include "TTT_HumanPlayer.h"
#include "TTT_RandomPlayer.h"
#include "TTT_MinmaxPlayer.h"
#include "EngineUtils.h"

ATTT_GameMode::ATTT_GameMode()
{
	PlayerControllerClass = ATTT_PlayerController::StaticClass();
	DefaultPawnClass = ATTT_HumanPlayer::StaticClass();
}

void ATTT_GameMode::BeginPlay()
{
	Super::BeginPlay();

	IsGameOver = false;

	MoveCounter = 0;

	//ATTT_HumanPlayer* HumanPlayer = *TActorIterator<ATTT_HumanPlayer>(GetWorld());
	ATTT_HumanPlayer* HumanPlayer = GetWorld()->GetFirstPlayerController()->GetPawn<ATTT_HumanPlayer>();

	if (!IsValid(HumanPlayer))
	{
		UE_LOG(LogTemp, Error, TEXT("No player pawn of type '%s' was found."), *ATTT_HumanPlayer::StaticClass()->GetName());
		return;
	}

	if (GridData)
	{
		FieldSize = GridData->GridSize;
		TileSize = GridData->TileSize;
		CellPadding = GridData->CellPadding;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GridData has not been assigned."));
		return;
	}

	if (GameFieldClass != nullptr)
	{
		GField = GetWorld()->SpawnActor<AGameField>(GameFieldClass);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Game Field is null"));
		return;
	}

	// La telecamera si posiziona matematicamente al centro della griglia (che ora è 25x25)
	float CameraPosX = ((TileSize * FieldSize) + ((FieldSize - 1) * TileSize * CellPadding)) * 0.5f;

	// NOTA: Con una griglia 25x25, un'altezza (Z) di 4000.0f potrebbe risultare un po' troppo vicina. 
	// Se vedi la mappa tagliata, prova ad aumentare questo valore (es. 8000.0f o 10000.0f)
	FVector CameraPos(CameraPosX, CameraPosX, 4000.0f);
	HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());


	// Human player = 0
	Players.Add(HumanPlayer);

	TSubclassOf<APawn> AIClassToSpawn = GetWorld()->GetGameInstance<UTTT_GameInstance>()->SelectedAIClass;

	auto* AI = GetWorld()->SpawnActor<ITTT_PlayerInterface>(AIClassToSpawn);

	// Random Player
	//auto* AI = GetWorld()->SpawnActor<ATTT_RandomPlayer>(FVector(), FRotator());

	// MiniMax Player
	//auto* AI = GetWorld()->SpawnActor<ATTT_MinmaxPlayer>(FVector(), FRotator());

	// AI player = 1
	Players.Add(AI);

	this->ChoosePlayerAndStartGame();
}

void ATTT_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// ANNULLA IL TIMER!
	GetWorld()->GetTimerManager().ClearTimer(ResetTimerHandle);
}

void ATTT_GameMode::ChoosePlayerAndStartGame()
{
	CurrentPlayer = FMath::RandRange(0, Players.Num() - 1);

	for (int32 IndexI = 0; IndexI < Players.Num(); IndexI++)
	{
		Players[IndexI]->PlayerNumber = IndexI;
		Players[IndexI]->Sign = IndexI == CurrentPlayer ? ESign::X : ESign::O;
	}
	MoveCounter += 1;
	Players[CurrentPlayer]->OnTurn();
}

void ATTT_GameMode::SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition)
{
	if (IsGameOver || PlayerNumber != CurrentPlayer)
	{
		return;
	}

	// TEMPORANEO: Manteniamo lo spawn dei segni X/O per capire se il click funziona.
	// In futuro, questa funzione verrà rinominata/modificata per gestire il movimento 
	// o lo spawn delle Unità (Sniper e Brawler) invece che dei semplici "segni".
	UClass* SignActor = Players[CurrentPlayer]->Sign == ESign::X ? SignXActor : SignOActor;
	FVector Location = GField->GetActorLocation() + SpawnPosition + FVector(0, 0, 10);
	GetWorld()->SpawnActor(SignActor, &Location);

	// RIMOSSO IL VECCHIO CONTROLLO DEL TRIS (IsWinPosition)
	// Adesso ci limitiamo a passare il turno. La logica di fine partita
	// delle 3 Torri la implementeremo quando creeremo la Macchina a Stati.

	TurnNextPlayer();
}

int32 ATTT_GameMode::GetNextPlayer(int32 Player)
{
	Player++;
	if (!Players.IsValidIndex(Player))
	{
		Player = 0;
	}
	return Player;
}

void ATTT_GameMode::TurnNextPlayer()
{
	MoveCounter += 1;
	CurrentPlayer = GetNextPlayer(CurrentPlayer);
	Players[CurrentPlayer]->OnTurn();
}
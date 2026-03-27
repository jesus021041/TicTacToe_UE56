// Fill out your copyright notice in the Description page of Project Settings.

#include "TTT_GameMode.h"
#include "TTT_PlayerController.h"
#include "TTT_HumanPlayer.h"
#include "TTT_RandomPlayer.h"
#include "EngineUtils.h"

ATTT_GameMode::ATTT_GameMode()
{
	PlayerControllerClass = ATTT_PlayerController::StaticClass();
	DefaultPawnClass = ATTT_HumanPlayer::StaticClass();
}

void ATTT_GameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=== INIZIO PARTITA ==="));

	IsGameOver = false;
	MoveCounter = 0;
	Player0UnitsPlaced = 0;
	Player1UnitsPlaced = 0;

	CurrentGameState = EGameState::CoinFlip;

	ATTT_HumanPlayer* HumanPlayer = GetWorld()->GetFirstPlayerController()->GetPawn<ATTT_HumanPlayer>();

	if (!IsValid(HumanPlayer))
	{
		UE_LOG(LogTemp, Error, TEXT("No player pawn found."));
		return;
	}

	if (GridData)
	{
		FieldSize = GridData->GridSize;
		TileSize = GridData->TileSize;
		CellPadding = GridData->CellPadding;
	}

	if (GameFieldClass != nullptr)
	{
		GField = GetWorld()->SpawnActor<AGameField>(GameFieldClass);
		UE_LOG(LogTemp, Warning, TEXT("GameField Spawnato con successo."));
	}

	float CameraPosX = ((TileSize * FieldSize) + ((FieldSize - 1) * TileSize * CellPadding)) * 0.5f;
	FVector CameraPos(CameraPosX, CameraPosX, 4000.0f);
	HumanPlayer->SetActorLocationAndRotation(CameraPos, FRotationMatrix::MakeFromX(FVector(0, 0, -1)).Rotator());

	Players.Add(HumanPlayer);

	TSubclassOf<APawn> AIClassToSpawn = GetWorld()->GetGameInstance<UTTT_GameInstance>()->SelectedAIClass;
	auto* AI = GetWorld()->SpawnActor<ITTT_PlayerInterface>(AIClassToSpawn);
	Players.Add(AI);

	PerformCoinFlip();
}

void ATTT_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(ResetTimerHandle);
}

void ATTT_GameMode::PerformCoinFlip()
{
	// FIX FONDAMENTALE: Assegniamo i PlayerNumber prima di iniziare!
	// Senza questo, l'IA pensava di essere il Giocatore 0 e il GameMode rifiutava le sue mosse!
	for (int32 i = 0; i < Players.Num(); i++)
	{
		if (Players[i])
		{
			Players[i]->PlayerNumber = i;
		}
	}

	StartingPlayer = FMath::RandRange(0, 1);
	CurrentPlayer = StartingPlayer;

	UE_LOG(LogTemp, Warning, TEXT("=== LANCIO MONETA ESEGUITO ==="));
	UE_LOG(LogTemp, Warning, TEXT("Ha vinto il Giocatore: %d (0 = Umano, 1 = IA)"), CurrentPlayer);

	if (CurrentPlayer == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("LANCIO MONETA: Inizi tu a piazzare!"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, TEXT("LANCIO MONETA: L'IA inizia a piazzare!"));
	}

	CurrentGameState = EGameState::Placement;
	Players[CurrentPlayer]->OnTurn();
}

void ATTT_GameMode::SetCellSign(const int32 PlayerNumber, const FVector& SpawnPosition)
{
	UE_LOG(LogTemp, Warning, TEXT("-> SetCellSign chiamato dal Player %d"), PlayerNumber);

	if (IsGameOver || PlayerNumber != CurrentPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Azione rifiutata. IsGameOver: %d, Player che ci prova: %d, Player di turno: %d"), IsGameOver, PlayerNumber, CurrentPlayer);
		return;
	}

	FVector2D GridPosRaw = GField->GetXYPositionByRelativeLocation(SpawnPosition);

	// Arrotondiamo i float per trovare con precisione le celle nella TMap
	FVector2D GridPos;
	GridPos.X = FMath::RoundToDouble(GridPosRaw.X);
	GridPos.Y = FMath::RoundToDouble(GridPosRaw.Y);

	UE_LOG(LogTemp, Warning, TEXT("Posizione convertita in Grid (X, Y): (%f, %f)"), GridPos.X, GridPos.Y);

	// --- FASE 1: PIAZZAMENTO ---
	if (CurrentGameState == EGameState::Placement)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stato attuale: PIAZZAMENTO. Il Player %d sta cercando di piazzare."), CurrentPlayer);

		// REQUISITO PDF: Limitazione Aree di Schieramento
		if (CurrentPlayer == 0 && GridPos.Y > 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("Azione rifiutata: L'umano ha cercato di piazzare oltre Y=2 (La sua Y era %f)"), GridPos.Y);
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Umano: Piazza solo nelle righe Y=0, 1, 2!"));
			Players[CurrentPlayer]->OnTurn(); // Rifà il turno senza avanzare
			return;
		}
		if (CurrentPlayer == 1 && GridPos.Y < 22)
		{
			UE_LOG(LogTemp, Warning, TEXT("Azione rifiutata: L'IA ha cercato di piazzare sotto Y=22 (La sua Y era %f). Riprova."), GridPos.Y);
			Players[CurrentPlayer]->OnTurn();
			return;
		}

		int32& UnitsPlaced = (CurrentPlayer == 0) ? Player0UnitsPlaced : Player1UnitsPlaced;

		// La prima unità è lo Sniper, la seconda il Brawler
		TSubclassOf<ABaseUnit> UnitToSpawn = (UnitsPlaced == 0) ? SniperClass : BrawlerClass;

		if (!UnitToSpawn)
		{
			UE_LOG(LogTemp, Error, TEXT("ERRORE CRITICO: UnitToSpawn è vuota! Non hai assegnato la Blueprint (SniperClass o BrawlerClass) nell'editor di Unreal!"));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ERRORE CRITICO: Assegna Sniper/Brawler Class nel BP!"));
		}

		if (UnitToSpawn && GField)
		{
			ATile* TargetTile = nullptr;
			if (GField->TileMap.Contains(GridPos))
			{
				TargetTile = GField->TileMap[GridPos];
				UE_LOG(LogTemp, Warning, TEXT("Cella bersaglio trovata con successo nella TileMap!"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ERRORE: Impossibile trovare la cella (%f, %f) nella TileMap!"), GridPos.X, GridPos.Y);
			}

			if (TargetTile)
			{
				FVector Location = TargetTile->GetActorLocation(); // Prendiamo la Z reale del terreno!
				Location.Z += 60.0f; // Solleviamo la pedina dal suolo

				UE_LOG(LogTemp, Warning, TEXT("Tentativo di spawn Actor in posizione: %s"), *Location.ToString());

				ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(UnitToSpawn, Location, FRotator::ZeroRotator);
				if (NewUnit)
				{
					NewUnit->PlayerOwner = CurrentPlayer;
					UE_LOG(LogTemp, Warning, TEXT("Spawn Avvenuto con successo per il Player %d!"), CurrentPlayer);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("ERRORE DURANTE LO SPAWN DELL'ACTOR! (Problema di collisioni o classe errata)"));
				}

				// Blocchiamo la cella
				TargetTile->SetTileStatus(CurrentPlayer, ETileStatus::OCCUPIED);

				UnitsPlaced++;
				FString UnitName = (UnitsPlaced == 1) ? TEXT("Sniper") : TEXT("Brawler");
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("Giocatore %d ha piazzato: %s"), CurrentPlayer, *UnitName));
			}
		}

		// Se entrambi hanno piazzato 2 unità, Inizia la Battaglia!
		if (Player0UnitsPlaced == 2 && Player1UnitsPlaced == 2)
		{
			CurrentGameState = EGameState::Playing;
			UE_LOG(LogTemp, Warning, TEXT("=== FASE PIAZZAMENTO CONCLUSA - INIZIA LA BATTAGLIA ==="));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Emerald, TEXT("PIAZZAMENTO CONCLUSO! Inizia la battaglia."));

			// Il primo turno vero va a chi ha vinto la moneta
			CurrentPlayer = StartingPlayer;
			Players[CurrentPlayer]->OnTurn();
			return;
		}

		TurnNextPlayer();
	}
	// --- FASE 2: GIOCO (Da implementare in seguito) ---
	else if (CurrentGameState == EGameState::Playing)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fase GIOCO, il player %d passa la mossa."), CurrentPlayer);
		TurnNextPlayer();
	}
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
	UE_LOG(LogTemp, Warning, TEXT("---- PASSO IL TURNO AL GIOCATORE %d ----"), CurrentPlayer);
	Players[CurrentPlayer]->OnTurn();
}
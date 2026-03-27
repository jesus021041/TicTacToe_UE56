// Fill out your copyright notice in the Description page of Project Settings.

#include "TTT_RandomPlayer.h"
#include "TTT_GameMode.h"
#include "GameField.h"
#include "Tile.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ATTT_RandomPlayer::ATTT_RandomPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	GameInstance = Cast<UTTT_GameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
}

// Called when the game starts or when spawned
void ATTT_RandomPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void ATTT_RandomPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// ANNULLA IL TIMER per evitare crash se il mondo viene distrutto mentre l'IA "pensa"
	GetWorld()->GetTimerManager().ClearTimer(AI_TurnTimerHandle);
}

// Called every frame
void ATTT_RandomPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATTT_RandomPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATTT_RandomPlayer::OnTurn()
{
	// Diamo all'IA un secondo e mezzo di tempo per "pensare", così il gioco risulta più naturale
	GetWorld()->GetTimerManager().SetTimer(AI_TurnTimerHandle, [&]()
		{
			ATTT_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
			if (!GameMode || !GameMode->GField) return;

			// --- FASE 1: PIAZZAMENTO DELLE UNITA' ---
			if (GameMode->CurrentGameState == EGameState::Placement)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("IA: Sto piazzando un'unita'..."));

				TArray<ATile*> ValidPlacementCells;

				// Cerchiamo le celle valide per l'IA
				for (auto& CurrTile : GameMode->GField->GetTileArray())
				{
					FVector2D Pos = CurrTile->GetGridPosition();

					// REQUISITO PDF: L'IA può piazzare solo nelle righe Y = 22, 23, 24.
					// Inoltre, la cella deve essere vuota (ricorda che l'acqua e le torri sono già settate su OCCUPIED nel GameField)
					if (CurrTile->GetTileStatus() == ETileStatus::EMPTY && Pos.Y >= 22)
					{
						ValidPlacementCells.Add(CurrTile);
					}
				}

				// Se c'è almeno una cella valida, l'IA ne sceglie una a caso
				if (ValidPlacementCells.Num() > 0)
				{
					int32 RandIdx = FMath::RandRange(0, ValidPlacementCells.Num() - 1);
					ATile* ChosenTile = ValidPlacementCells[RandIdx];

					FVector2D GridPos = ChosenTile->GetGridPosition();
					FVector RelativeLoc = GameMode->GField->GetRelativeLocationByXYPosition(GridPos.X, GridPos.Y);

					// In futuro qui faremo lo spawn fisico dello Sniper o del Brawler dell'IA.
					// Per ora usiamo SetCellSign per far avanzare la macchina a stati e passare il turno.
					GameMode->SetCellSign(PlayerNumber, RelativeLoc);
				}
			}
			// --- FASE 2: GIOCO EFFETTIVO (Movimento e Attacco) ---
			else if (GameMode->CurrentGameState == EGameState::Playing)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("IA: E' il mio turno di muovere!"));

				// TODO: Qui in futuro implementeremo l'algoritmo A* per far muovere l'IA

				// Per ora, passiamo semplicemente il turno al giocatore umano per non bloccare la partita
				GameMode->TurnNextPlayer();
			}

		}, 1.5f, false);
}

void ATTT_RandomPlayer::OnWin()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("L'IA ha Vinto!"));
}

void ATTT_RandomPlayer::OnLose()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("L'IA ha Perso!"));
}
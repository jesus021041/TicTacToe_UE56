// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "ActionLogWidget.generated.h"

UCLASS()
class TICTACTOE_UE56_API UActionLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Il box che conterrà tutti i messaggi scorrevoli
	UPROPERTY(meta = (BindWidget))
	UScrollBox* LogScrollBox;

	UFUNCTION(BlueprintCallable, Category = "Action Log")
	void AddLogMessage(const FString& Message, FLinearColor Color = FLinearColor::White);
};
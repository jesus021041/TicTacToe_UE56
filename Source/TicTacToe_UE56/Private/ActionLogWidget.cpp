// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionLogWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"

void UActionLogWidget::AddLogMessage(const FString& Message, FLinearColor Color)
{
	// Sicurezza: se il box nn esiste nella UI -> si interrompe
	if (!LogScrollBox) return;

	// Creiamo dinamicamente una nuova riga di testo passando lo ScrollBox come Parent
	UTextBlock* NewLogLine = NewObject<UTextBlock>(LogScrollBox);
	if (NewLogLine)
	{
		// Impostiamo il msg e il colore passato come parametro
		NewLogLine->SetText(FText::FromString(Message));
		NewLogLine->SetColorAndOpacity(FSlateColor(Color));

		// Impostiamo una dimensione leggibile per il font => 14
		FSlateFontInfo FontInfo = NewLogLine->GetFont();
		FontInfo.Size = 14;
		NewLogLine->SetFont(FontInfo);

		// Fa anndare a capo nei msg
		NewLogLine->SetAutoWrapText(true);

		// Aggiungiamo il testo in fondo allo ScrollBox
		LogScrollBox->AddChild(NewLogLine);

		// Scorriamo automaticamente verso il basso per mostrare sempre l'ultima mossa eseguita
		LogScrollBox->ScrollToEnd();
	}
}
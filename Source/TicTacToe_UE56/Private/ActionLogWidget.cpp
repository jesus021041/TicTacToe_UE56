// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionLogWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"
#include "Components/ScrollBoxSlot.h"

void UActionLogWidget::AddLogMessage(const FString& Message, FLinearColor Color)
{
	// Sicurezza: se il box nn esiste nella UI -> si interrompe
	if (!LogScrollBox) return;

	// Creiamo dinamicamente una nuova riga di testo
	UTextBlock* NewLogLine = NewObject<UTextBlock>(LogScrollBox);
	if (NewLogLine)
	{
		UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(LogScrollBox->AddChild(NewLogLine));

		// stile testuale & colori
		NewLogLine->SetText(FText::FromString(Message));

		//Applichiamo il colore
		NewLogLine->SetColorAndOpacity(FSlateColor(Color));

		// Impostiamo una dimensione leggibile per il font => 16
		FSlateFontInfo FontInfo = NewLogLine->GetFont();
		FontInfo.Size = 16;
		FontInfo.TypefaceFontName = FName("Bold");
		NewLogLine->SetFont(FontInfo);

		// Fa anndare a capo nei msg
		NewLogLine->SetAutoWrapText(true);

		//Margini
		if (NewSlot)
		{
			NewSlot->SetPadding(FMargin(10.0f, 5.0f, 10.0f, 5.0f));
		}

		// Scorriamo automaticamente verso il basso per mostrare sempre l'ultima mossa eseguita
		LogScrollBox->ScrollToEnd();
	}
}
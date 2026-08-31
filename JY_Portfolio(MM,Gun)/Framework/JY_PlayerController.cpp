// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/JY_PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Core/JY.h"
#include "Framework/JY_PlayerState.h"
#include "Components/JY_AbilitySystemComponent.h"
#include "Components/JY_WeaponStateComponent.h"
#include "UI/JY_CrosshairWidget.h"

AJY_PlayerController::AJY_PlayerController()
{
	WeaponStateComponent = CreateDefaultSubobject<UJY_WeaponStateComponent>(TEXT("WeaponStateComponent"));
}

void AJY_PlayerController::BeginPlay() //override
{
	Super::BeginPlay();

	/* [Owning Client] 크로스헤어는 화면 전용, 로컬 PlayerController에서만 생성 */
	if (IsLocalPlayerController() == true && CrosshairWidgetClass != nullptr)
	{
		CrosshairWidget = CreateWidget<UJY_CrosshairWidget>(this, CrosshairWidgetClass);
		if (CrosshairWidget != nullptr)
		{
			CrosshairWidget->AddToPlayerScreen(10);
			CrosshairWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

/* [Owning Client] 로컬 크로스헤어 표시/숨김 */
void AJY_PlayerController::SetCrosshairVisible(bool bVisible)
{
	if (CrosshairWidget == nullptr)
	{
		return;
	}

	CrosshairWidget->SetVisibility(bVisible == true ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

/* [Server -> Client RPC] 명중 확인한 서버가 쏜 사람 클라에만 통보 */
void AJY_PlayerController::ClientPlayHitMarker_Implementation()
{
	if (CrosshairWidget == nullptr)
	{
		return;
	}

	CrosshairWidget->PlayHitFlash();
}

void AJY_PlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (AJY_PlayerState* PS = GetPlayerState<AJY_PlayerState>())
	{
		if (UJY_AbilitySystemComponent* ASC = PS->GetJYAbilitySystemComponent())
		{
			ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

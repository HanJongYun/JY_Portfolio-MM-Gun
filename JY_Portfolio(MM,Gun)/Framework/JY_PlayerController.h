// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JY_PlayerController.generated.h"

class UJY_WeaponStateComponent;
class UJY_CrosshairWidget;

UCLASS(abstract)
class AJY_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AJY_PlayerController();

public:
	void SetCrosshairVisible(bool bVisible);

	/* [Server -> Client RPC] 명중 확인 시 호출, 크로스헤어 명중 표시 재생 */
	UFUNCTION(Client, Reliable)
	void ClientPlayHitMarker();

protected:
	virtual void BeginPlay() override;

	/* 매 프레임 입력 처리 후 호출, ASC 어빌리티 입력 큐 소비 */
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

protected:
	/* 현재 무기 확산 상태 갱신 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JY|Weapon")
	TObjectPtr<UJY_WeaponStateComponent> WeaponStateComponent;

	/* 화면 중앙 크로스헤어 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "JY|UI|Crosshair")
	TSubclassOf<UJY_CrosshairWidget> CrosshairWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UJY_CrosshairWidget> CrosshairWidget;
};

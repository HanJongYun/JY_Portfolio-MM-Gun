#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "JY_AbilitySystemComponent.generated.h"

UCLASS()
class UJY_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UJY_AbilitySystemComponent();

public:
	/* DynamicSpecSourceTags에 InputTag 있어야 매칭 (AbilitySet이 심음) */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/* PlayerController::PostProcessInput에서 매 프레임 호출 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	/* 사망/빙의 해제 등에서 호출 */
	void ClearAbilityInput();

private:
	/* 이번 프레임 눌린 핸들 */
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	/* 이번 프레임 떼어진 핸들 */
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	/* 유지 중인 핸들 (WhileInputActive) */
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};

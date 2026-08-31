#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "JY_AbilitySet.generated.h"

class UJY_GameplayAbility;
class UJY_AbilitySystemComponent;

USTRUCT(BlueprintType)
struct FJYAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UJY_GameplayAbility> Ability = nullptr;

	/* 유효하면 그랜트 시 DynamicSpecSourceTags에 심어 입력 매칭에 사용 */
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;
};

/* 부여된 어빌리티 핸들 모음, 무기 해제/슬롯 교체 시 회수용 */
USTRUCT(BlueprintType)
struct FJYAbilitySet_GrantedHandles
{
	GENERATED_BODY()

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);

	/* [Server] 기록한 어빌리티 전부 ClearAbility로 회수, 목록 비움 */
	void TakeFromAbilitySystem(UJY_AbilitySystemComponent* ASC);

private:

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
};

UCLASS(BlueprintType, Const)
class UJY_AbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:

	/* [Server] 어빌리티 부여 */
	void GiveToAbilitySystem(UJY_AbilitySystemComponent* ASC, FJYAbilitySet_GrantedHandles* OutGrantedHandles = nullptr, UObject* SourceObject = nullptr) const;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities", Meta = (TitleProperty = "Ability"))
	TArray<FJYAbilitySet_GameplayAbility> GrantedGameplayAbilities;
};

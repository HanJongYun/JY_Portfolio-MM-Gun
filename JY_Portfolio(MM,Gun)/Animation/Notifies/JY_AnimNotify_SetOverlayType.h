// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Core/JY_Types.h"
#include "JY_AnimNotify_SetOverlayType.generated.h"

UCLASS(meta = (DisplayName = "JY Set Overlay Type"))
class UJY_AnimNotify_SetOverlayType : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

public:
	UPROPERTY(EditAnywhere, Category = "JY|Notify")
	EJY_WeaponAnimType OverlayType = EJY_WeaponAnimType::Unarmed;
};

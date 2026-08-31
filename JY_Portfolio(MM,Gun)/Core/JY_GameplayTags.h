#pragma once

#include "NativeGameplayTags.h"

namespace JYGameplayTags
{
	// 입력 - 네이티브 (C++ 함수 직접 바인딩: 이동/시점)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Stick);

	// 입력 - 어빌리티 (GAS AbilityInputTagPressed 경유)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Prone);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Equip_Slot1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Equip_Slot2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Aim);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_FireAuto);

	// 연출 - GameplayCue (재질별 분기는 태그가 아니라 노티파이의 SurfaceType 필터가 담당)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_Impact);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_Fire);

	// 어빌리티 상태 (ActivationOwnedTags - 어빌리티 활성 중 ASC에 자동 부여·복제되는 태그)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Firing);
}

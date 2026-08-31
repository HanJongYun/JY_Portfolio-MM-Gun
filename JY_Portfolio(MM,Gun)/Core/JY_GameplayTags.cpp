#include "Core/JY_GameplayTags.h"

namespace JYGameplayTags
{
	// 입력 - 네이티브
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move,       "InputTag.Move",       "이동 입력. Axis2D: X=좌우, Y=전후");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "시점 입력(마우스). Axis2D: X=Yaw, Y=Pitch");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Stick, "InputTag.Look.Stick", "시점 입력(게임패드 스틱). Axis2D: X=Yaw, Y=Pitch");

	// 입력 - 어빌리티
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump,   "InputTag.Jump",   "점프 입력. GAS 어빌리티 경유.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch", "앉기 입력. HeroComponent 네이티브 입력으로 처리.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Prone,  "InputTag.Prone",  "엎드리기 입력. HeroComponent 네이티브 입력으로 처리.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Equip_Slot1, "InputTag.Equip.Slot1", "무기 슬롯0 직접 선택(키 1). HeroComponent 네이티브 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Equip_Slot2, "InputTag.Equip.Slot2", "무기 슬롯1 직접 선택(키 2). HeroComponent 네이티브 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Aim,         "InputTag.Aim",         "조준(ADS) 입력. HeroComponent 네이티브 입력으로 처리(Pressed/Released).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Fire, "InputTag.Weapon.Fire", "무기 발사 입력. GAS 어빌리티 경유.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_FireAuto, "InputTag.Weapon.FireAuto", "자동 무기 연사 입력. GAS 어빌리티 경유.");

	// 연출 - GameplayCue
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Impact, "GameplayCue.Weapon.Impact", "총알 명중 타격 연출. 노티파이가 재질(SurfaceType)별 파티클·소리·카메라 흔들림을 고른다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Fire, "GameplayCue.Weapon.Fire", "발사 순간 연출. 명중과 무관하게 한 발마다 카메라 흔들림·총구 화염·발사음을 낸다.");

	// 어빌리티 상태
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Firing, "Ability.Weapon.Firing", "WeaponFire 어빌리티 활성 중 ASC에 자동 부여되는 상태 태그. 다른 시스템이 '지금 발사 중'을 조회할 때 쓴다.");
}

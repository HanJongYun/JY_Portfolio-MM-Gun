// Copyright Epic Games, Inc. All Rights Reserved.

/* GASP 콘텐츠(BP/ABP)가 GetConsoleVariable로 읽는 DDCvar.*가 원본 C++ 등록 없이 매 프레임 경고를 냄.
   여기서 이름만 등록(기본값 0)해 경고만 제거, 기능 변화 없음. 실제로 켜려면 해당 값을 1로 */

#include "HAL/IConsoleManager.h"

// 캐릭터/폰
static TAutoConsoleVariable<int32> CVarDD_PawnClass(
	TEXT("DDCvar.PawnClass"), 0,
	TEXT("[JY] 마이그레이션 GASP. 기본 off"), ECVF_Default);

static TAutoConsoleVariable<int32> CVarDD_VisualOverride(
	TEXT("DDCvar.VisualOverride"), 0,
	TEXT("[JY] 마이그레이션 GASP 비주얼 오버라이드. 기본 off"), ECVF_Default);

// OffsetRootBone
static TAutoConsoleVariable<float> CVarDD_OffsetRootBoneTranslationRadius(
	TEXT("DDCvar.OffsetRootBone.TranslationRadius"), 0.f,
	TEXT("[JY] 마이그레이션 OffsetRootBone 디버그 반경. 기본 off"), ECVF_Default);

// 트래버설
static TAutoConsoleVariable<int32> CVarDD_TraversalDrawDebugLevel(
	TEXT("DDCvar.Traversal.DrawDebugLevel"), 0,
	TEXT("[JY] 마이그레이션 트래버설 디버그 드로우 레벨. 기본 off"), ECVF_Default);

// 디버그 드로우류
static TAutoConsoleVariable<int32> CVarDD_DrawVisLogShapesForFoleySounds(
	TEXT("DDCvar.DrawVisLogShapesForFoleySounds"), 0,
	TEXT("[JY] 마이그레이션 폴리 사운드 VisLog 셰이프. 기본 off"), ECVF_Default);

static TAutoConsoleVariable<int32> CVarDD_DrawCharacterDebugShapes(
	TEXT("DDCvar.DrawCharacterDebugShapes"), 0,
	TEXT("[JY] 마이그레이션 캐릭터 디버그 셰이프. 기본 off"), ECVF_Default);

static TAutoConsoleVariable<int32> CVarDD_DrawCharacterDebugStates(
	TEXT("DDCvar.DrawCharacterDebugStates"), 0,
	TEXT("[JY] 마이그레이션 캐릭터 디버그 상태. 기본 off"), ECVF_Default);

static TAutoConsoleVariable<int32> CVarDD_DrawCharacterDebugGraphs(
	TEXT("DDCvar.DrawCharacterDebugGraphs"), 0,
	TEXT("[JY] 마이그레이션 캐릭터 디버그 그래프. 기본 off"), ECVF_Default);

// 기능 토글(.Enable), 로그엔 DDCVar(대문자 V)로 떴음, 켜고 싶으면 1로
static TAutoConsoleVariable<int32> CVarDD_ThreadSafeAnimationUpdateEnable(
	TEXT("DDCVar.ThreadSafeAnimationUpdate.Enable"), 0,
	TEXT("[JY] 마이그레이션 스레드세이프 애님 업데이트. 기본 off(원하면 1)"), ECVF_Default);

static TAutoConsoleVariable<int32> CVarDD_NewGameplayCameraSystemEnable(
	TEXT("DDCVar.NewGameplayCameraSystem.Enable"), 0,
	TEXT("[JY] 마이그레이션 신규 게임플레이 카메라 시스템. 기본 off(원하면 1)"), ECVF_Default);

// 2차로 PIE에서 더 뜬 것들
static TAutoConsoleVariable<int32> CVarDD_LocomotionSetupCMC(
	TEXT("DDcvar.LocomotionSetupCMC"), 0,
	TEXT("[JY] 마이그레이션 GASP 로코모션 셋업(CMC). 기본 off(원하면 1)"), ECVF_Default);

static TAutoConsoleVariable<int32> CVarDD_UseUnrealAnimationFramework(
	TEXT("DDcvar.UseUnrealAnimationFramework"), 0,
	TEXT("[JY] 마이그레이션 GASP UAF 사용 토글. 기본 off(원하면 1)"), ECVF_Default);

static TAutoConsoleVariable<float> CVarDD_TraversalDrawDebugDuration(
	TEXT("DDcvar.Traversal.DrawDebugDuration"), 0.f,
	TEXT("[JY] 마이그레이션 트래버설 디버그 드로우 지속시간. 기본 0"), ECVF_Default);

/* 다른 DDCvar.* 경고 뜨면 같은 패턴으로 추가 */

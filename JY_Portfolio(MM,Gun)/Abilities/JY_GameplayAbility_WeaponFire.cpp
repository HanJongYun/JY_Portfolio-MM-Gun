#include "Abilities/JY_GameplayAbility_WeaponFire.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Actors/JY_WeaponActor.h"
#include "Ballistics/JY_BallisticSubsystem.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Utils/JY_WeaponUtils.h"
#include "Core/JY_GameplayTags.h"
#include "JY_CollisionChannels.h"
#include "Data/JY_WeaponData.h"
#include "Equipment/JY_EquipmentInstance.h"
#include "Equipment/JY_WeaponInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "Actors/JY_Character.h"
#include "Components/JY_AimComponent.h"
#include "Framework/JY_PlayerController.h"

UJY_GameplayAbility_WeaponFire::UJY_GameplayAbility_WeaponFire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationPolicy = EJYAbilityActivationPolicy::WhileInputActive;
	FireDelayTimeSecs = 0.12f;
	ImpactGameplayCueTag = JYGameplayTags::GameplayCue_Weapon_Impact;
	FireGameplayCueTag = JYGameplayTags::GameplayCue_Weapon_Fire;
	ActivationOwnedTags.AddTag(JYGameplayTags::Ability_Weapon_Firing);
}

bool UJY_GameplayAbility_WeaponFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) == false)
		return false;

	const AJY_Character* Character = ActorInfo != nullptr ? Cast<AJY_Character>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UJY_AimComponent* AimComponent = Character != nullptr ? Character->GetAimComponent() : nullptr;
	if (AimComponent != nullptr && (AimComponent->IsAiming() == false || AimComponent->IsWallAhead() == true))
		return false;

	return true;
}

void UJY_GameplayAbility_WeaponFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) //override
{
	UJY_EquipmentInstance* EquipmentInstance = GetAssociatedEquipment();
	if (EquipmentInstance == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetDataReadyCallbackDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APawn* Pawn = EquipmentInstance->GetPawn();

	if (Pawn != nullptr && Pawn->IsLocallyControlled())
	{
		StartWeaponTargeting();
	}

	if (IsActive() == false)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float SafeFireDelayTimeSecs = FMath::Max(FireDelayTimeSecs, UE_KINDA_SMALL_NUMBER);
	World->GetTimerManager().SetTimer(FireDelayTimerHandle, this, &ThisClass::HandleFireDelayFinished, SafeFireDelayTimeSecs, false);
}

void UJY_GameplayAbility_WeaponFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) //override
{
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(FireDelayTimerHandle);
	}

	UAbilitySystemComponent* ASC = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC != nullptr)
	{
		ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).Remove(TargetDataReadyCallbackDelegateHandle);
		ASC->ConsumeClientReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey());
	}

	TargetDataReadyCallbackDelegateHandle.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UJY_GameplayAbility_WeaponFire::HandleFireDelayFinished()
{
	if (IsActive() == false)
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UJY_GameplayAbility_WeaponFire::StartWeaponTargeting()
{
	UAbilitySystemComponent* ASC = CurrentActorInfo != nullptr ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC == nullptr)
	{
		K2_CancelAbility();
		return;
	}

	AJY_WeaponActor* WeaponActor = GetWeaponActor();
	UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	if (WeaponData == nullptr)
	{
		K2_CancelAbility();
		return;
	}

	TArray<FHitResult> HitResults;
	const bool bTargetingSucceeded = WeaponData->FireMode == EJY_WeaponFireMode::Ballistic
		? PerformBallisticTargeting(OUT HitResults)
		: PerformHitscanTargeting(OUT HitResults);

	if (bTargetingSucceeded == false)
	{
		K2_CancelAbility();
		return;
	}

	FScopedPredictionWindow ScopedPrediction(ASC, CurrentActivationInfo.GetActivationPredictionKey());

	FGameplayAbilityTargetDataHandle TargetData;
	for (const FHitResult& HitResult : HitResults)
	{
		FGameplayAbilityTargetData_SingleTargetHit* SingleTargetHit = new FGameplayAbilityTargetData_SingleTargetHit();
		SingleTargetHit->HitResult = HitResult;
		TargetData.Add(SingleTargetHit);
	}

	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}

bool UJY_GameplayAbility_WeaponFire::PerformBallisticTargeting(TArray<FHitResult>& OutHitResults) const
{
	/* [Owning Client] */
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AJY_WeaponActor* WeaponActor = GetWeaponActor();
	UJY_WeaponInstance* WeaponInstance = Cast<UJY_WeaponInstance>(GetAssociatedEquipment());
	UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false || WeaponInstance == nullptr || WeaponData == nullptr)
		return false;

	const FTransform TargetTransform = GetTargetingTransform();
	const FVector StartLocation = TargetTransform.GetTranslation();
	const FVector AimDirection = TargetTransform.GetUnitAxis(EAxis::X);
	const float HalfSpreadAngleRadians = FMath::DegreesToRadians(WeaponInstance->GetCalculatedSpreadAngle() * 0.5f);
	const FVector BulletDirection = JY_WeaponUtils::GetRandomDirectionInSpreadCone(AimDirection, HalfSpreadAngleRadians, WeaponInstance->GetSpreadExponent());

	/* 시작점/방향만 전달 -> JY_BallisticSubsystem에서 히트 판단 */
	FHitResult BallisticStartData;
	BallisticStartData.TraceStart = StartLocation;
	BallisticStartData.TraceEnd = StartLocation + BulletDirection * WeaponData->BallisticSettings.MaxRange;
	/* 발사 큐가 트레이서 끝점/총구 화염 방향으로 읽는 좌표. 탄도 트레이서 구현 전까지 최대사거리 직선 끝점을 임시 기입 */
	BallisticStartData.Location = BallisticStartData.TraceEnd;

	OutHitResults.Reset();
	OutHitResults.Add(BallisticStartData);

	return true;
}


bool UJY_GameplayAbility_WeaponFire::RegisterBallisticShot(const FGameplayAbilityTargetDataHandle& TargetData)
{
	/* [Owning Client] */
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AJY_WeaponActor* WeaponActor = GetWeaponActor();
	UJY_WeaponInstance* WeaponInstance = Cast<UJY_WeaponInstance>(GetAssociatedEquipment());
	UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	UWorld* World = GetWorld();
	const FGameplayAbilityTargetData* TargetDataEntry = TargetData.Num() > 0 ? TargetData.Get(0) : nullptr;
	const FHitResult* BallisticStartData = TargetDataEntry != nullptr ? TargetDataEntry->GetHitResult() : nullptr;
	if (Pawn == nullptr || WeaponInstance == nullptr || WeaponData == nullptr || World == nullptr || BallisticStartData == nullptr)
		return false;

	const FVector BulletDirection = (BallisticStartData->TraceEnd - BallisticStartData->TraceStart).GetSafeNormal();
	if (BulletDirection.IsNearlyZero() == true)
		return false;

	UJY_BallisticSubsystem* BallisticSubsystem = World->GetSubsystem<UJY_BallisticSubsystem>();
	if (BallisticSubsystem == nullptr)
		return false;

	/* 피격 보고 시 사용할 예측키 지정 */
	FJY_BallisticShot Shot;
	Shot.PredictionKey = CurrentActivationInfo.GetActivationPredictionKey();
	Shot.Position = BallisticStartData->TraceStart;
	Shot.Velocity = BulletDirection * WeaponData->BallisticSettings.InitialSpeed;
	Shot.Settings = WeaponData->BallisticSettings;
	Shot.Shooter = Pawn;
	Shot.WeaponInstance = WeaponInstance;
	Shot.RemainingPenetrationPower = WeaponData->PenetrationPower;
	Shot.PenetrationCostBySurface = WeaponData->PenetrationCostBySurface;
	BallisticSubsystem->AddBallisticShot(MoveTemp(Shot));

	return true;
}

void UJY_GameplayAbility_WeaponFire::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* ASC = CurrentActorInfo != nullptr ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC == nullptr)
	{
		K2_CancelAbility();
		return;
	}

	FScopedPredictionWindow ScopedPrediction(ASC);
	FGameplayAbilityTargetDataHandle LocalTargetData(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

	const bool bIsLocallyControlled = CurrentActorInfo->IsLocallyControlled();
	const bool bIsServer = CurrentActorInfo->IsNetAuthority();

	if (bIsLocallyControlled == true && bIsServer == false)
	{
		/* [Owning Client -> Server RPC] */
		ASC->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetData, ApplicationTag, ASC->ScopedPredictionKey);
	}

	if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UJY_WeaponInstance* WeaponInstance = Cast<UJY_WeaponInstance>(GetAssociatedEquipment());
	if (WeaponInstance == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	WeaponInstance->AddSpread();

	AJY_WeaponActor* WeaponActor = GetWeaponActor();
	UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	const bool bIsBallistic = WeaponData != nullptr && WeaponData->FireMode == EJY_WeaponFireMode::Ballistic;

	ExecuteFireCue(LocalTargetData);

	if (bIsBallistic == true)
	{
		/* [Owning Client] 로컬 탄도 시뮬레이션에만 등록, 명중 판단과 데미지는 JY_BallisticSubsystem 에서 처리 */
		if (bIsLocallyControlled == true && RegisterBallisticShot(LocalTargetData) == false)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}
	}
	else
	{
		ExecuteImpactCues(LocalTargetData);

		if (bIsServer == true)
		{
			ApplyDamageToHits(LocalTargetData);
		}
	}
}

void UJY_GameplayAbility_WeaponFire::ExecuteImpactCues(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (ImpactGameplayCueTag.IsValid() == false)
		return;

	for (int32 HitIndex = 0; HitIndex < TargetData.Num(); ++HitIndex)
	{
		const FGameplayAbilityTargetData* TargetDataEntry = TargetData.Get(HitIndex);
		const FHitResult* HitResult = TargetDataEntry != nullptr ? TargetDataEntry->GetHitResult() : nullptr;
		if (HitResult == nullptr || HitResult->GetComponent() == nullptr)
			continue;

		FGameplayCueParameters CueParameters;
		CueParameters.Location = HitResult->ImpactPoint;
		CueParameters.Normal = HitResult->ImpactNormal;
		CueParameters.PhysicalMaterial = HitResult->PhysMaterial;
		K2_ExecuteGameplayCueWithParams(ImpactGameplayCueTag, CueParameters);
	}
}

void UJY_GameplayAbility_WeaponFire::ApplyDamageToHits(const FGameplayAbilityTargetDataHandle& TargetData)
{
	/* [Server] */
	AActor* ShooterActor = GetAvatarActorFromActorInfo();
	AJY_WeaponActor* WeaponActor = GetWeaponActor();

	bool bAnyHitConfirmed = false;
	for (int32 HitIndex = 0; HitIndex < TargetData.Num(); ++HitIndex)
	{
		const FGameplayAbilityTargetData* TargetDataEntry = TargetData.Get(HitIndex);
		const FHitResult* HitResult = TargetDataEntry != nullptr ? TargetDataEntry->GetHitResult() : nullptr;

		if (HitResult == nullptr || HitResult->GetComponent() == nullptr)
		{
			continue;
		}

		if (JY_WeaponUtils::ApplyWeaponDamage(ShooterActor, WeaponActor, *HitResult, GetAbilityLevel()) == true)
		{
			bAnyHitConfirmed = true;
		}
	}

	if (bAnyHitConfirmed == true)
	{
		if (AJY_PlayerController* PC = Cast<AJY_PlayerController>(GetActorInfo().PlayerController.Get()))
		{
			/* [Server -> Client RPC] */
			PC->ClientPlayHitMarker();
		}
	}
}

void UJY_GameplayAbility_WeaponFire::ExecuteFireCue(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (FireGameplayCueTag.IsValid() == false)
		return;

	FGameplayCueParameters CueParameters;

	CueParameters.Instigator = GetAvatarActorFromActorInfo();

	for (int32 HitIndex = 0; HitIndex < TargetData.Num(); ++HitIndex)
	{
		const FGameplayAbilityTargetData* Entry = TargetData.Get(HitIndex);
		const FHitResult* Hit = Entry != nullptr ? Entry->GetHitResult() : nullptr;
		if (Hit == nullptr)
			continue;

		CueParameters.Location = Hit->GetComponent() != nullptr ? Hit->ImpactPoint : Hit->Location;
	}

	if (AJY_WeaponActor* WeaponActor = GetWeaponActor())
	{
		CueParameters.TargetAttachComponent = WeaponActor->GetMeshComponent();
	}

	K2_ExecuteGameplayCueWithParams(FireGameplayCueTag, CueParameters);
}

AJY_WeaponActor* UJY_GameplayAbility_WeaponFire::GetWeaponActor() const
{
	UJY_EquipmentInstance* EquipmentInstance = GetAssociatedEquipment();
	if (EquipmentInstance == nullptr)
		return nullptr;

	for (AActor* SpawnedActor : EquipmentInstance->GetSpawnedActors())
	{
		if (AJY_WeaponActor* WeaponActor = Cast<AJY_WeaponActor>(SpawnedActor))
			return WeaponActor;
	}

	return nullptr;
}

FVector UJY_GameplayAbility_WeaponFire::GetWeaponTargetingSourceLocation() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (Pawn == nullptr)
		return FVector::ZeroVector;

	return Pawn->GetActorLocation();
}

FTransform UJY_GameplayAbility_WeaponFire::GetTargetingTransform() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AController* Controller = Pawn != nullptr ? Pawn->GetController() : nullptr;
	if (Pawn == nullptr || Controller == nullptr)
		return FTransform::Identity;

	FVector CameraLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(OUT CameraLocation, OUT ViewRotation);

	/* GetPlayerViewPoint의 회전은 쉐이크가 적용되어 있으므로 ControlRotation 사용 */
	const FRotator AimRotation = Controller->GetControlRotation();
	const FVector AimDirection = AimRotation.Vector().GetSafeNormal();

	const float FocalDistance = 1024.f;
	const FVector FocalLocation = CameraLocation + AimDirection * FocalDistance;
	const FVector WeaponLocation = GetWeaponTargetingSourceLocation();
	const float WeaponDepthAlongAim = FVector::DotProduct(WeaponLocation - FocalLocation, AimDirection);
	const FVector StartLocation = FocalLocation + AimDirection * WeaponDepthAlongAim;
	return FTransform(AimRotation, StartLocation);
}

bool UJY_GameplayAbility_WeaponFire::PerformHitscanTargeting(TArray<FHitResult>& OutHitResults) const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AJY_WeaponActor* WeaponActor = GetWeaponActor();
	UJY_WeaponInstance* WeaponInstance = Cast<UJY_WeaponInstance>(GetAssociatedEquipment());
	UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	AController* Controller = Pawn != nullptr ? Pawn->GetController() : nullptr;
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false || WeaponActor == nullptr || WeaponInstance == nullptr || WeaponData == nullptr || Controller == nullptr)
		return false;

	const FTransform TargetTransform = GetTargetingTransform();
	const FVector StartTrace = TargetTransform.GetTranslation();
	const FVector AimDirection = TargetTransform.GetUnitAxis(EAxis::X);
	const float HalfSpreadAngleRadians = FMath::DegreesToRadians(WeaponInstance->GetCalculatedSpreadAngle() * 0.5f);
	const FVector BulletDirection = JY_WeaponUtils::GetRandomDirectionInSpreadCone(AimDirection, HalfSpreadAngleRadians, WeaponInstance->GetSpreadExponent());
	const FVector EndTrace = StartTrace + BulletDirection * WeaponData->MaxTraceRange;
	if (WeaponSweepWithSurfacePenetration(StartTrace, EndTrace, WeaponData->BulletTraceRadius, OUT OutHitResults) == false)
		return false;

	return true;
}

bool UJY_GameplayAbility_WeaponFire::WeaponSweepWithSurfacePenetration(const FVector& StartTrace, const FVector& EndTrace, float TraceRadius, TArray<FHitResult>& OutHitResults) const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (Pawn == nullptr || GetWorld() == nullptr)
		return false;

	OutHitResults.Reset();

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(JYWeaponPenetrationTrace), true, Pawn);
	TraceParams.bReturnPhysicalMaterial = true;

	TArray<AActor*> AttachedActors;
	Pawn->GetAttachedActors(OUT AttachedActors);
	TraceParams.AddIgnoredActors(AttachedActors);

	const FVector TraceDirection = (EndTrace - StartTrace).GetSafeNormal();
	if (TraceDirection.IsNearlyZero() == true)
		return false;

	AJY_WeaponActor* WeaponActor = GetWeaponActor();
	UJY_WeaponData* WeaponData = WeaponActor != nullptr ? WeaponActor->GetWeaponData() : nullptr;
	if (WeaponData == nullptr)
		return false;

	const FCollisionShape BulletShape = FCollisionShape::MakeSphere(FMath::Max(TraceRadius, 0.1f));
	TArray<FHitResult> SweptHits;
	GetWorld()->SweepMultiByChannel(OUT SweptHits, StartTrace, EndTrace, FQuat::Identity, JY_TraceChannel::HitScan, BulletShape, TraceParams);

	/* 빗나감, 사거리 끝점 반환 */
	if (SweptHits.Num() == 0)
	{
		FHitResult MissResult;
		MissResult.TraceStart = StartTrace;
		MissResult.TraceEnd = EndTrace;
		MissResult.Location = EndTrace;
		MissResult.ImpactPoint = EndTrace;
		OutHitResults.Add(MissResult);
		return true;
	}

	/* 표면별 관통 비용 차감, 소진되면 중단 */
	float RemainingPower = WeaponData->PenetrationPower;
	for (const FHitResult& Hit : SweptHits)
	{
		OutHitResults.Add(Hit);

		const EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
		const bool bShouldStop = JY_WeaponUtils::ApplyPenetrationCost(WeaponData->GetPenetrationCost(SurfaceType), OUT RemainingPower, OutHitResults.Num());
		if (bShouldStop == true)
			break;
	}

	return true;
}

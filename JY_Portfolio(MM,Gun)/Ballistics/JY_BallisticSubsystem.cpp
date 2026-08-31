#include "Ballistics/JY_BallisticSubsystem.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Actors/JY_BaseCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Utils/JY_WeaponUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Core/JY_GameplayTags.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "GameplayCueManager.h"
#include "GameplayEffectTypes.h"
#include "JY_CollisionChannels.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(JY_BallisticSubsystem)

namespace JYBallistic
{
	static TAutoConsoleVariable<float> CVarDrawPathDuration(TEXT("jy.Ballistic.DrawPathDuration"), 2.f, TEXT("Ballistic path debug draw duration. 0 disables drawing."), ECVF_Cheat);

	static TAutoConsoleVariable<float> CVarFixedTimeStep(TEXT("jy.Ballistic.FixedTimeStep"), 1.f / 120.f, TEXT("Ballistic fixed simulation time step in seconds."), ECVF_Cheat);

	static TAutoConsoleVariable<int32> CVarMaxSimulationStepsPerFrame(TEXT("jy.Ballistic.MaxSimulationStepsPerFrame"), 8, TEXT("Maximum ballistic simulation steps per frame."), ECVF_Cheat);
}

void UJY_BallisticSubsystem::Deinitialize()
{
	ActiveShots.Reset();
	NextShotID = 1;
	SimulationTimeAccumulator = 0.f;

	Super::Deinitialize();
}

void UJY_BallisticSubsystem::Tick(float DeltaTime)
{
	/* 플레이어 발사분 기준 클라 전용, 몬스터의 경우 서버 전용 */
	Super::Tick(DeltaTime);

	if (DeltaTime <= 0.f)
		return;

	const float FixedTimeStep = FMath::Max(JYBallistic::CVarFixedTimeStep.GetValueOnGameThread(), UE_SMALL_NUMBER);
	const int32 MaxSimulationStepsPerFrame = FMath::Max(JYBallistic::CVarMaxSimulationStepsPerFrame.GetValueOnGameThread(), 1);

	/* 프레임 급증 시 과도한 반복 방지 */
	const float MaxAccumulatedTime = FixedTimeStep * MaxSimulationStepsPerFrame;
	SimulationTimeAccumulator = FMath::Min(SimulationTimeAccumulator + DeltaTime, MaxAccumulatedTime);

	int32 SimulationStepCount = 0;
	while (SimulationTimeAccumulator >= FixedTimeStep && SimulationStepCount < MaxSimulationStepsPerFrame && ActiveShots.IsEmpty() == false)
	{
		SimulationTimeAccumulator -= FixedTimeStep;
		SimulationStepCount++;

		/* 역순회로 안전하게 제거 */
		for (int32 ShotIndex = ActiveShots.Num() - 1; ShotIndex >= 0; --ShotIndex)
		{
			FJY_BallisticShot& Shot = ActiveShots[ShotIndex];
			const FVector PreviousPosition = Shot.Position;

			FVector NewPosition;
			FVector NewVelocity;
			IntegrateShotRK2(Shot.Position, Shot.Velocity, Shot.Settings, FixedTimeStep, OUT NewPosition, OUT NewVelocity);

			/* 발사자/부착장비/이미 관통한 표면 제외 */
			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(JYBallisticTrace), true);
			TraceParams.bReturnPhysicalMaterial = true;
			for (const TWeakObjectPtr<AActor>& IgnoredActor : Shot.IgnoredActors)
			{
				if (IgnoredActor.IsValid() == true)
				{
					TraceParams.AddIgnoredActor(IgnoredActor.Get());
				}
			}
			for (const TWeakObjectPtr<UPrimitiveComponent>& IgnoredComponent : Shot.IgnoredHitComponents)
			{
				if (IgnoredComponent.IsValid() == true)
				{
					TraceParams.AddIgnoredComponent(IgnoredComponent.Get());
				}
			}

			TArray<FHitResult> SweptHits;
			if (Shot.Settings.ProjectileRadius > 0.f)
			{
				const FCollisionShape ProjectileShape = FCollisionShape::MakeSphere(Shot.Settings.ProjectileRadius);
				GetWorld()->SweepMultiByChannel(OUT SweptHits, PreviousPosition, NewPosition, FQuat::Identity, JY_TraceChannel::HitScan, ProjectileShape, TraceParams);
			}
			else
			{
				GetWorld()->LineTraceMultiByChannel(OUT SweptHits, PreviousPosition, NewPosition, JY_TraceChannel::HitScan, TraceParams);
			}

			/* 표면별 관통 비용 차감, 소진되면 정지 */
			bool bShotStopped = false;
			for (const FHitResult& Hit : SweptHits)
			{
				UPrimitiveComponent* HitComponent = Hit.GetComponent();
				if (HitComponent == nullptr)
					continue;

				const TWeakObjectPtr<UPrimitiveComponent> WeakHitComponent(HitComponent);
				if (Shot.IgnoredHitComponents.Contains(WeakHitComponent) == true)
					continue;

				Shot.IgnoredHitComponents.Add(WeakHitComponent);
				Shot.ProcessedSurfaceCount++;

				const EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
				const float* FoundCost = Shot.PenetrationCostBySurface.Find(SurfaceType);
				const float PenetrationCost = FoundCost != nullptr ? *FoundCost : TNumericLimits<float>::Max();

				bShotStopped = JY_WeaponUtils::ApplyPenetrationCost(PenetrationCost, OUT Shot.RemainingPenetrationPower, Shot.ProcessedSurfaceCount);
				HandleBallisticImpact(Shot, Hit);

#if ENABLE_DRAW_DEBUG
				const float DrawDuration = JYBallistic::CVarDrawPathDuration.GetValueOnGameThread();
				if (DrawDuration > 0.f)
				{
					const FColor HitColor = bShotStopped == true ? FColor::Red : FColor::Blue;
					DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 8.f, 12, HitColor, false, DrawDuration, 0, 1.5f);
				}
#endif

				if (bShotStopped == false)
					continue;

				ActiveShots.RemoveAtSwap(ShotIndex, 1, EAllowShrinking::No);
				break;
			}

			if (bShotStopped == true)
				continue;

			Shot.Position = NewPosition;
			Shot.Velocity = NewVelocity;

			const float StepDistance = FVector::Distance(PreviousPosition, Shot.Position);
			Shot.ElapsedTime += FixedTimeStep;
			Shot.TraveledDistance += StepDistance;

#if ENABLE_DRAW_DEBUG
			const float DrawDuration = JYBallistic::CVarDrawPathDuration.GetValueOnGameThread();
			if (DrawDuration > 0.f)
			{
				DrawDebugLine(GetWorld(), PreviousPosition, Shot.Position, FColor::Green, false, DrawDuration, 0, 1.5f);
				DrawDebugPoint(GetWorld(), Shot.Position, 6.f, FColor::Yellow, false, DrawDuration);
			}
#endif

			const bool bExceededFlightTime = Shot.ElapsedTime >= Shot.Settings.MaxFlightTime;
			const bool bExceededRange = Shot.TraveledDistance >= Shot.Settings.MaxRange;
			if (bExceededFlightTime == true || bExceededRange == true)
			{
				ActiveShots.RemoveAtSwap(ShotIndex, 1, EAllowShrinking::No);
			}
		}
	}

	if (ActiveShots.IsEmpty() == true)
	{
		SimulationTimeAccumulator = 0.f;
	}
}

bool UJY_BallisticSubsystem::IsTickable() const
{
	return ActiveShots.IsEmpty() == false;
}

TStatId UJY_BallisticSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UJY_BallisticSubsystem, STATGROUP_Tickables);
}

uint32 UJY_BallisticSubsystem::AddBallisticShot(FJY_BallisticShot Shot)
{
	/* [Client/Server] */
	const uint32 AssignedShotID = NextShotID;
	NextShotID++;

	/* 0은 무효, 순환 시 1부터 재사용 */
	if (NextShotID == 0)
	{
		NextShotID = 1;
	}

	Shot.ShotID = AssignedShotID;
	Shot.ElapsedTime = 0.f;
	Shot.TraveledDistance = 0.f;
	Shot.ProcessedSurfaceCount = 0;
	Shot.IgnoredHitComponents.Reset();

	/* 발사자와 부착 장비는 경로에서 제외 */
	if (Shot.Shooter.IsValid() == true)
	{
		AActor* Shooter = Shot.Shooter.Get();
		Shot.IgnoredActors.AddUnique(TWeakObjectPtr<AActor>(Shooter));

		TArray<AActor*> AttachedActors;
		Shooter->GetAttachedActors(OUT AttachedActors);
		for (AActor* AttachedActor : AttachedActors)
		{
			if (AttachedActor != nullptr)
			{
				Shot.IgnoredActors.AddUnique(TWeakObjectPtr<AActor>(AttachedActor));
			}
		}
	}

	ActiveShots.Add(MoveTemp(Shot));

	return AssignedShotID;
}

FVector UJY_BallisticSubsystem::CalculateAcceleration(const FVector& Velocity, const FJY_BallisticSettings& Settings) const
{
	const FVector GravityAcceleration(0.f, 0.f, Settings.GravityZ);

	const float Speed = Velocity.Size();
	/* 속력 제곱에 비례하는 저항 (반대 방향) */
	const FVector DragAcceleration = -Settings.DragScale * Speed * Velocity;

	return GravityAcceleration + DragAcceleration;
}

void UJY_BallisticSubsystem::IntegrateShotRK2(const FVector& Position, const FVector& Velocity, const FJY_BallisticSettings& Settings, float DeltaTime, FVector& OutPosition, FVector& OutVelocity) const
{
	if (DeltaTime <= 0.f)
	{
		OutPosition = Position;
		OutVelocity = Velocity;
		return;
	}

	const FVector InitialAcceleration = CalculateAcceleration(Velocity, Settings);
	const FVector MidpointVelocity = Velocity + InitialAcceleration * (DeltaTime * 0.5f);
	const FVector MidpointAcceleration = CalculateAcceleration(MidpointVelocity, Settings);

	OutPosition = Position + MidpointVelocity * DeltaTime;
	OutVelocity = Velocity + MidpointAcceleration * DeltaTime;
}

void UJY_BallisticSubsystem::HandleBallisticImpact(const FJY_BallisticShot& Shot, const FHitResult& Hit) const
{
	/* [Owning Client] */
	AJY_BaseCharacter* BaseCharacter = Cast<AJY_BaseCharacter>(Shot.Shooter.Get());
	if (BaseCharacter == nullptr)
		return;

	BaseCharacter->ServerReportBallisticImpact(Hit, Shot.PredictionKey);

	UGameplayCueManager* GameplayCueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
	if (GameplayCueManager == nullptr)
		return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(BaseCharacter);
	if (ASC == nullptr)
		return;

	FGameplayCueParameters CueParameters;
	CueParameters.Location = Hit.ImpactPoint;
	CueParameters.Normal = Hit.ImpactNormal;
	CueParameters.PhysicalMaterial = Hit.PhysMaterial;

	GameplayCueManager->InvokeGameplayCueExecuted_WithParams(ASC, JYGameplayTags::GameplayCue_Weapon_Impact, Shot.PredictionKey, CueParameters);
}

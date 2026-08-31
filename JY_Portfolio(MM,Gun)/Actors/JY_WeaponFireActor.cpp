#include "Actors/JY_WeaponFireActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace JYWeaponFireActor
{
	static const FName ParamTrigger = TEXT("User.Trigger");
	static const FName ParamDirection = TEXT("User.Direction");
	static const FName ParamImpactPositions = TEXT("User.ImpactPositions");
	static const FName ParamShellMesh = TEXT("User.ShellEjectStaticMesh");

	static constexpr float LifetimeCheckIntervalSecs = 1.0f;
}

AJY_WeaponFireActor::AJY_WeaponFireActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRootComponent);
}

void AJY_WeaponFireActor::Initialize(USkeletalMeshComponent* InWeaponMesh, UNiagaraSystem* InMuzzleFlashSystem, UNiagaraSystem* InTracerSystem, UNiagaraSystem* InShellEjectSystem, UStaticMesh* InShellEjectMesh, FName InMuzzleSocketName, FName InShellEjectSocketName, USoundBase* InFireSound)
{
	WeaponMeshComponent = InWeaponMesh;
	MuzzleFlashSystem = InMuzzleFlashSystem;
	TracerSystem = InTracerSystem;
	ShellEjectSystem = InShellEjectSystem;
	ShellEjectMesh = InShellEjectMesh;
	MuzzleSocketName = InMuzzleSocketName;
	ShellEjectSocketName = InShellEjectSocketName;
	FireSound = InFireSound;
}

void AJY_WeaponFireActor::Fire(const TArray<FVector>& ImpactPositions)
{
	PlayShellEject();
	PlayMuzzleFlash(ImpactPositions);
	PlayTracer(ImpactPositions);
	PlayFireSound();

	EnsureLifetimePolling();
}

void AJY_WeaponFireActor::PlayMuzzleFlash(const TArray<FVector>& ImpactPositions)
{
	if (MuzzleFlashSystem == nullptr || WeaponMeshComponent == nullptr)
		return;

	UNiagaraComponent* Comp = SpawnOrReuseSystem(MuzzleFlashComponent, MuzzleFlashSystem, MuzzleSocketName);
	if (Comp == nullptr)
		return;

	const FVector MuzzleWorldLocation = WeaponMeshComponent->GetSocketLocation(MuzzleSocketName);
	const FVector ImpactPosition = ImpactPositions.Num() > 0 ? ImpactPositions[0] : MuzzleWorldLocation;
	const FVector FireDirection = (ImpactPosition - MuzzleWorldLocation).GetSafeNormal();
	Comp->SetVariableVec3(JYWeaponFireActor::ParamDirection, FireDirection);

	bMuzzleFlashTrigger = bMuzzleFlashTrigger == false;
	Comp->SetVariableBool(JYWeaponFireActor::ParamTrigger, bMuzzleFlashTrigger);
}

void AJY_WeaponFireActor::PlayTracer(const TArray<FVector>& ImpactPositions)
{
	if (TracerSystem == nullptr || WeaponMeshComponent == nullptr)
		return;

	UNiagaraComponent* Comp = SpawnOrReuseSystem(TracerComponent, TracerSystem, MuzzleSocketName);
	if (Comp == nullptr)
		return;

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(Comp, JYWeaponFireActor::ParamImpactPositions, ImpactPositions);

	bTracerTrigger = bTracerTrigger == false;
	Comp->SetVariableBool(JYWeaponFireActor::ParamTrigger, bTracerTrigger);
}

void AJY_WeaponFireActor::PlayShellEject()
{
	if (ShellEjectSystem == nullptr || WeaponMeshComponent == nullptr)
		return;

	UNiagaraComponent* Comp = SpawnOrReuseSystem(ShellEjectComponent, ShellEjectSystem, ShellEjectSocketName);
	if (Comp == nullptr)
		return;

	if (ShellEjectMesh != nullptr)
		Comp->SetVariableStaticMesh(JYWeaponFireActor::ParamShellMesh, ShellEjectMesh);

	bShellEjectTrigger = bShellEjectTrigger == false;
	Comp->SetVariableBool(JYWeaponFireActor::ParamTrigger, bShellEjectTrigger);
}

void AJY_WeaponFireActor::PlayFireSound()
{
	if (FireSound == nullptr || WeaponMeshComponent == nullptr)
		return;

	UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMeshComponent, MuzzleSocketName);
}

UNiagaraComponent* AJY_WeaponFireActor::SpawnOrReuseSystem(TObjectPtr<UNiagaraComponent>& InOutComp, UNiagaraSystem* System, FName SocketName)
{
	if (System == nullptr || WeaponMeshComponent == nullptr)
		return nullptr;

	if (IsValid(InOutComp) == true)
		return InOutComp;

	const FTransform SocketActorTransform = WeaponMeshComponent->GetSocketTransform(SocketName, RTS_Actor);
	InOutComp = UNiagaraFunctionLibrary::SpawnSystemAttached(System, SceneRootComponent, NAME_None, SocketActorTransform.GetLocation(), SocketActorTransform.GetRotation().Rotator(), EAttachLocation::KeepRelativeOffset, true, true);

	return InOutComp;
}

void AJY_WeaponFireActor::EnsureLifetimePolling()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	if (World->GetTimerManager().IsTimerActive(LifetimeTimerHandle) == true)
		return;

	World->GetTimerManager().SetTimer(LifetimeTimerHandle, this, &ThisClass::CheckLifetimeAndDestroy, JYWeaponFireActor::LifetimeCheckIntervalSecs, true);
}

void AJY_WeaponFireActor::CheckLifetimeAndDestroy()
{
	const bool bMuzzleActive = IsValid(MuzzleFlashComponent) == true && MuzzleFlashComponent->IsActive() == true;
	const bool bTracerActive = IsValid(TracerComponent) == true && TracerComponent->IsActive() == true;
	const bool bShellActive = IsValid(ShellEjectComponent) == true && ShellEjectComponent->IsActive() == true;

	if (bMuzzleActive == true || bTracerActive == true || bShellActive == true)
		return;

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);

	Destroy();
}

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JY_WeaponFireActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UStaticMesh;
class USoundBase;

UCLASS()
class AJY_WeaponFireActor : public AActor
{
	GENERATED_BODY()

public:
	AJY_WeaponFireActor();

public:
	void Initialize(USkeletalMeshComponent* InWeaponMesh, UNiagaraSystem* InMuzzleFlashSystem, UNiagaraSystem* InTracerSystem, UNiagaraSystem* InShellEjectSystem, UStaticMesh* InShellEjectMesh, FName InMuzzleSocketName, FName InShellEjectSocketName, USoundBase* InFireSound);
	void Fire(const TArray<FVector>& ImpactPositions);

private:
	void PlayMuzzleFlash(const TArray<FVector>& ImpactPositions);
	void PlayTracer(const TArray<FVector>& ImpactPositions);
	void PlayShellEject();
	void PlayFireSound();

	UNiagaraComponent* SpawnOrReuseSystem(TObjectPtr<UNiagaraComponent>& InOutComp, UNiagaraSystem* System, FName SocketName);

	void EnsureLifetimePolling();
	void CheckLifetimeAndDestroy();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> MuzzleFlashSystem;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> TracerSystem;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ShellEjectSystem;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ShellEjectMesh;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> FireSound;

	FName MuzzleSocketName = NAME_None;
	FName ShellEjectSocketName = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> MuzzleFlashComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TracerComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ShellEjectComponent;

	/** 매 발 반전해 나이아가라 `User.Trigger`로 넘기는 값 */
	bool bMuzzleFlashTrigger = false;
	bool bTracerTrigger = false;
	bool bShellEjectTrigger = false;

	FTimerHandle LifetimeTimerHandle;
};

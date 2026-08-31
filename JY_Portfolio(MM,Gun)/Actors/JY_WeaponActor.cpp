#include "Actors/JY_WeaponActor.h"

#include "Data/JY_WeaponData.h"
#include "Data/JY_WeaponGlobalData.h"
#include "Core/JY_GlobalSettings.h"
#include "Actors/JY_WeaponFireActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AJY_WeaponActor::AJY_WeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AJY_WeaponActor::BeginPlay()
{
	Super::BeginPlay();
}

void AJY_WeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJY_WeaponActor, WeaponType);
}

void AJY_WeaponActor::InitializeFromType(EJY_WeaponAnimType InType)
{
	/* [Server] */
	if (HasAuthority() == false)
	{
		return;
	}

	WeaponType = InType;
	ApplyWeaponType();
}

bool AJY_WeaponActor::TryGetMuzzleTransform(FTransform& OutMuzzleTransform) const
{
	if (MeshComp == nullptr || CachedData == nullptr || CachedData->MuzzleSocketName.IsNone() == true || MeshComp->DoesSocketExist(CachedData->MuzzleSocketName) == false)
		return false;

	OutMuzzleTransform = MeshComp->GetSocketTransform(CachedData->MuzzleSocketName, RTS_World);
	return true;
}

void AJY_WeaponActor::PlayFireEffects(const TArray<FVector>& ImpactPositions)
{
	/* [Server 포함 전 클라] */
	if (CachedData == nullptr || MeshComp == nullptr)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	if (IsValid(WeaponFire) == false)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		WeaponFire = World->SpawnActor<AJY_WeaponFireActor>(AJY_WeaponFireActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (WeaponFire == nullptr)
			return;

		WeaponFire->AttachToComponent(MeshComp, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		WeaponFire->Initialize(MeshComp, CachedData->MuzzleFlashSystem, CachedData->TracerSystem, CachedData->ShellEjectSystem, CachedData->ShellEjectMesh, CachedData->MuzzleSocketName, CachedData->ShellEjectSocketName, CachedData->FireSound);
	}

	WeaponFire->Fire(ImpactPositions);
}

void AJY_WeaponActor::OnRep_WeaponType()
{
	/* [Client] */
	ApplyWeaponType();
}

void AJY_WeaponActor::ApplyWeaponType()
{
	const UJY_GlobalSettings* Settings = GetDefault<UJY_GlobalSettings>();
	UJY_WeaponGlobalData* GlobalData = (Settings != nullptr) ? Settings->WeaponGlobalData.LoadSynchronous() : nullptr;
	CachedData = (GlobalData != nullptr) ? GlobalData->GetWeaponData(WeaponType) : nullptr;

	if (CachedData == nullptr)
	{
		return;
	}

	USkeletalMesh* LoadedMesh = CachedData->WeaponMesh.LoadSynchronous();
	if (LoadedMesh != nullptr)
	{
		MeshComp->SetSkeletalMesh(LoadedMesh);
	}
}

#include "GameplayCues/JY_GCN_WeaponFire.h"

#include "Actors/JY_WeaponActor.h"
#include "Components/SceneComponent.h"

bool UJY_GCN_WeaponFire::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	Super::OnExecute_Implementation(Target, Parameters);

	/* Fire 큐가 TargetAttachComponent에 넣어준 무기 메시의 소유자가 무기 액터 */
	USceneComponent* AttachComp = Parameters.TargetAttachComponent.Get();
	if (AttachComp == nullptr)
		return true;

	AJY_WeaponActor* Weapon = Cast<AJY_WeaponActor>(AttachComp->GetOwner());
	if (Weapon == nullptr)
		return true;

	TArray<FVector> ImpactPositions;
	ImpactPositions.Add(Parameters.Location);

	/* 발사 연출(총구/궤적/탄피) 위임 */
	Weapon->PlayFireEffects(ImpactPositions);

	return true;
}

#include "Utils/JY_Utils.h"

#include "GameFramework/Actor.h"

FString JYDebugUtils::GetNetRoleString(const AActor* Actor)
{
	if (Actor == nullptr)
	{
		return TEXT("None");
	}

	switch (Actor->GetLocalRole())
	{
	case ROLE_Authority:       return Actor->GetNetMode() == NM_Standalone ? TEXT("Standalone") : TEXT("Server");
	case ROLE_AutonomousProxy: return TEXT("OwnClient");
	case ROLE_SimulatedProxy:  return TEXT("SimProxy");
	default:                   return TEXT("Unknown");
	}
}

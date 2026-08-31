#pragma once

class AActor;

namespace JYDebugUtils
{
	/* 액터 네트워크 역할 문자열 변환(Standalone/Server/OwnClient/SimProxy) */
	FString GetNetRoleString(const AActor* Actor);
}

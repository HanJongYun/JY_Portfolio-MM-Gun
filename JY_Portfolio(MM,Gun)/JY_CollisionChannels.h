#pragma once

#include "Engine/EngineTypes.h"

namespace JY_TraceChannel
{
	/** 파쿠르/매달리기 판정용. */
	constexpr ECollisionChannel Traversable = ECC_GameTraceChannel1;

	/** 근접 공격 판정용. */
	constexpr ECollisionChannel Attack = ECC_GameTraceChannel2;

	/** 총알 히트스캔 판정용. 기본 응답 Overlap(관통 경로를 한 번에 긁기 위함), 백스톱만 Block. */
	constexpr ECollisionChannel HitScan = ECC_GameTraceChannel3;
}

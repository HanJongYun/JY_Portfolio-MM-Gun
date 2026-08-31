#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JY_CrosshairWidget.generated.h"

UCLASS()
class UJY_CrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* 서버가 이 클라 탄 명중 통보 시 호출, 색 반짝임은 위젯 블루프린트에서 구현 */
	UFUNCTION(BlueprintImplementableEvent, Category = "JY|Crosshair")
	void PlayHitFlash();
};

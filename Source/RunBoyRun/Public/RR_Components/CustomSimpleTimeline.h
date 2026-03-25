// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine.h"
#include "UObject/NoExportTypes.h"
#include "Curves/CurveFloat.h"
#include "Engine/EngineTypes.h"
#include "CustomSimpleTimeline.generated.h"

/**
 * 
 */
// 재생 방향 정보
UENUM(BlueprintType)
enum class ECustomSimpleTimelineDirection : uint8
{
	Forward	UMETA(DisplayName = "Forward"),
	Backward UMETA(DisplayName = "Backward")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSimpleTimerDelegate,	// 델리게이트 타입의 이름
	float, CurrentValue,	 // 파라미터 1: float 타입, 이름 "CurrentValue"
	ECustomSimpleTimelineDirection, Direction	// 파라미터 2: Enum 타입, 이름 "Direction"
);

UCLASS(BlueprintType)
class RUNBOYRUN_API UCustomSimpleTimeline : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simplified Timeline", Meta = (ExposeOnSpawn = true))
	float PlaybackFrequency = 100;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Simplified Timeline", Meta = (ExposeOnSpawn = true))
	UCurveFloat* Curve;

	// ★ 출력 데이터 핀 역할: Update 이벤트 
	// BlueprintAssignable = 블루프린트에서 바인딩 가능 
	// Category = "Timeline Events" = 블루프린트 상세 패널 분류
	UPROPERTY(BlueprintAssignable, Category = "Simplified Timeline")
	FSimpleTimerDelegate OnTimerUpdate;

	// ★ 출력 데이터 핀 역할: Finished 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Simplified Timeline")
	FSimpleTimerDelegate OnTimerFinished;

	// Functions

	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	void Play(UObject* WorldContextObject);

	// 항상 처음부터 재생 시작
	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	void PlayFromStart(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	void Unpause();

	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	void Stop();

	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	void Reverse();

	// 현재 재생 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	bool IsPlaying() const { return bIsPlaying; }

	// 현재 시간 반환
	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	float GetCurrentTime() const { return CurrTime; }

	// 총 재생 시간 반환
	UFUNCTION(BlueprintCallable, Category = "Simplified Timeline")
	float GetTotalDuration() const { return CurveEndTime - CurveStartTime; }

private:
	float UpdateInterval = 0.01f;

	ECustomSimpleTimelineDirection CurrentDirection = ECustomSimpleTimelineDirection::Forward;

	FTimerHandle TimerUpdateHandle;

	UWorld* CachedWorld = nullptr;

	bool bIsPlaying = false;

	float CurrTime = 0.0f;
	float CurveStartTime = 0.0f;
	float CurveEndTime = 0.0f;
	float CurrentValue = 0.0f;

	void StartTimer();

	UFUNCTION(Category = "Simplified Timeline")
	void UpdateTimer();

	void InitializeTimeline();

	// 유효한 월드 포인터 반환 (안정성 체크 포함
	UWorld* GetValidWorld() const;

	// 진행률 계산 ( 0 ~ 1 )
	float GetProgress() const;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "RR_Components/CustomSimpleTimeline.h"



void UCustomSimpleTimeline::Play(UObject* WorldContextObject)
{
	CurrentDirection = ECustomSimpleTimelineDirection::Forward;

	if (!bIsPlaying)
	{
		CachedWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		if (CachedWorld != nullptr && Curve != nullptr)
			StartTimer();

		return;
	}
	else
	{
		switch (CurrentDirection)
		{
		case ECustomSimpleTimelineDirection::Forward:
			Unpause();
			break;
		case ECustomSimpleTimelineDirection::Backward:
			Pause();
			CurrentDirection = ECustomSimpleTimelineDirection::Forward;
			break;
		default:
			break;
		}
	}
}

void UCustomSimpleTimeline::PlayFromStart(UObject* WorldContextObject)
{
	CurrentDirection = ECustomSimpleTimelineDirection::Forward;

	// PlayFromStart의 핵심: 항상 처음부터 시작
	InitializeTimeline();        // Curve에서 시간 범위 읽기
	CurrTime = CurveStartTime;  // 0초로 강제 리셋 (가장 중요!)

	// Play()와 동일한 로직 시작
	if (!bIsPlaying)
	{
		// 처음 재생 시작인 경우
		CachedWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		if (CachedWorld != nullptr && Curve != nullptr)
		{
			StartTimer(); // 0초부터 타이머 시작
			bIsPlaying = true;
		}
		return;
	}
	else
	{
		// 이미 재생 중인 경우 : 처음부터 다시 시작
		if (CachedWorld != nullptr && Curve != nullptr)
		{
			// 기존 타이머 정리
			if (CachedWorld->GetTimerManager().IsTimerActive(TimerUpdateHandle))
			{
				CachedWorld->GetTimerManager().ClearTimer(TimerUpdateHandle);
			}

			// 새 타이머 시작 (0초부터)
			StartTimer();
		}
	}
}

void UCustomSimpleTimeline::StartTimer()
{ 
	// setup start and finish time.
	Curve->GetTimeRange(CurveStartTime, CurveEndTime);

	switch (CurrentDirection)
	{
	case ECustomSimpleTimelineDirection::Forward:
		CurrTime = CurveStartTime;
		break;
	case ECustomSimpleTimelineDirection::Backward:
		CurrTime = CurveEndTime;
		break;
	default:
		break;
	}

	// 주기에 따라 업데이트 간격 설정
	UpdateInterval = 1.0 / PlaybackFrequency;

	CachedWorld->GetTimerManager().ClearAllTimersForObject(this);
	CachedWorld->GetTimerManager().SetTimer(
		TimerUpdateHandle,
		this,
		&UCustomSimpleTimeline::UpdateTimer,
		UpdateInterval,
		true
	);

	bIsPlaying = true;
}

void UCustomSimpleTimeline::Pause()
{
	if (bIsPlaying)
	{
		if(!CachedWorld->GetTimerManager().IsTimerPaused(TimerUpdateHandle))
			CachedWorld->GetTimerManager().PauseTimer(TimerUpdateHandle);
	}
}

void UCustomSimpleTimeline::Unpause()
{
	if (bIsPlaying)
	{
		if (CachedWorld->GetTimerManager().IsTimerPaused(TimerUpdateHandle))
			CachedWorld->GetTimerManager().UnPauseTimer(TimerUpdateHandle);
	}
}



void UCustomSimpleTimeline::Stop()
{
	if (bIsPlaying)
	{
		if (CachedWorld != nullptr)
		{
			if (CachedWorld->GetTimerManager().TimerExists(TimerUpdateHandle))
				CachedWorld->GetTimerManager().ClearTimer(TimerUpdateHandle);

			bIsPlaying = false;
		}
	}
}



void UCustomSimpleTimeline::Reverse()
{
	CurrentDirection = ECustomSimpleTimelineDirection::Backward;
	if (!bIsPlaying)
	{
		if (CachedWorld != nullptr && Curve != nullptr)
		{
			CurrTime = CurveEndTime;
			StartTimer();
		}
	}
}


void UCustomSimpleTimeline::UpdateTimer()
{
	if (!bIsPlaying)
		return;

	bool bHasFinished = false;

	switch (CurrentDirection)
	{
	case ECustomSimpleTimelineDirection::Forward:
		CurrTime += UpdateInterval;
		if (CurrTime >= CurveEndTime)
		{
			CurrTime = CurveEndTime;
			bHasFinished = true;
		}
		break;
	case ECustomSimpleTimelineDirection::Backward:
		CurrTime -= UpdateInterval;
		if (CurrTime <= CurveStartTime)
		{
			CurrTime = CurveStartTime;
			bHasFinished = true;
		}
		break;
	default:
		break;
	}

	CurrentValue = Curve->GetFloatValue(CurrTime);

	OnTimerUpdate.Broadcast(CurrentValue, CurrentDirection);

	if (bHasFinished)
	{
		Stop();
		OnTimerFinished.Broadcast(CurrentValue, CurrentDirection);
		// 동작: 
		// 1. OnTimelineUpdate에 바인딩된 모든 이벤트를 찾음 
		// 2. 각 이벤트에 (CurrentValue, CurrentDirection) 파라미터 전달 
		// 3. 모든 이벤트 함수 실행		
	}
}

void UCustomSimpleTimeline::InitializeTimeline()
{
	if (Curve != nullptr)
	{
		Curve->GetTimeRange(CurveStartTime, CurveEndTime);
	}
}


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

/**
 *
 */

// 클라이밍 상태 관리 델리게이트 타입 선언
DECLARE_DELEGATE(FOnEnterClimbState);
DECLARE_DELEGATE(FOnExitClimbState);


class UAnimMontage;
class UAnimInstance;
class ARunBoyRunCharacter;
class UMotionWarpingComponent;

// 커스텀 무브먼트 모드를 정의하는 열거형 (Namespace로 충돌 방지)
UENUM(BlueprintType)
namespace ECustomMovementMode
{
	enum Type
	{
		MOVE_Climb UMETA(DisplayName = "Climb Mode")

	};
}

// Trace 함수에 축 선택을 위한 열거형 정의 
UENUM(BlueprintType)
enum class ETraceDirectionAxis : uint8
{
	Up		UMETA(DisplayName = "Up"),
	Right	UMETA(DisplayName = "Right"),
	Forward UMETA(DisplayName = "Forward"),
	Custom	UMETA(DisplayName = "Custom")
};


UCLASS()
class RUNBOYRUN_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	// 클라이밍 상태 관리 델리게이트 멤버 변수 생성
	FOnEnterClimbState OnEnterClimbStateDelegate;
	FOnExitClimbState OnExitClimbStateDelegate;



protected:

#pragma region OverridenFunction

	virtual void BeginPlay() override;

	// 매 프레임 컴포넌트 업데이트
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 무브먼트 모드가 변경될 때 엔진에 의해 자동으로 호출되는 함수
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	// MOVE_Custom 상태일 때 엔진이 자동으로 호출하는 함수
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;

	// 루트 모션의 속도 적용 방식을 제어하는 가상 함수 
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;

	// 안전한 모션 워핑 컴포넌트 캐싱 및 늦은 바인딩 반환 함수
	UMotionWarpingComponent* GetCachedMotionWarpingComponent();

#pragma endregion


private:

#pragma region Climb Traces
	// 다중 캡슐 트레이스를 수행하고 충돌 결과 배열을 반환하는 헬퍼 함수 
	TArray<FHitResult> DoCapsuleTraceMultiByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);
	
	FHitResult DoLineTraceSingleByObject(const FVector& Start, const FVector& End, bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

#pragma endregion

#pragma region ClimbCore

	// 전방의 등반 가능한 표면을 감지하는 메인 함수
	bool TraceClimbableSurfaces();
	
	// 시선 높이에서 트레이스를 발사하는 메인 함수
	FHitResult TraceFromEyeHeight(
		float TraceDistance, 
		float TraceStartOffset = 0.f,
		ETraceDirectionAxis AxisType = ETraceDirectionAxis::Up,
		bool bShowDebugShape = false, 
		bool bDrawPersistantShapes = false,
		FVector CustomDirection = FVector::ZeroVector  // 대각선 커스텀 방향용
	);

	// 클라이밍 시작 조건 검사
	bool CanStartClimbing();

	// 절벽아래로 매달릴 수 있는 상황인지 판별
	bool CanClimbDownLedge();

	void StartClimbing();

	void StopClimbing();

	// 실질적인 클라이밍 물리 로직이 들어갈 헬퍼 함수
	void PhysClimb(float deltaTime, int32 Iterations);

	// 표면 데이터를 취합하고 평균을 내는 함수
	void ProcessClimableSurfaceInfo();

	// 바닥이나 경사가 완만한 표면에 도달했는지 확인하는 함수 
	bool CheckShouldStopClimbing();

	// 바닥에 도달했는지 판별하는 함수
	bool CheckHasReachedFloor();

	// 클라이밍시 부드러운 회전 처리
	FQuat GetClimbRotation(float DeltaTime);

	// 벽 표면 스냅 처리 로직
	void SnapMovementToClimableSurfaces(float DeltaTime);

	// 절벽 꼭대기(Ledge)에 도달했는지 판별하는 함수 
	bool CheckHasReachedLedge();
	
	// Vaulting 시도 로직
	void TryStartVaulting();

	// Vaulting 가능 검사 로직 
	bool CanStartvaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition);

	void PlayClimbMontage(UAnimMontage* MontageToPlay);

	// 델리게이트 요구 조건(Signature)에 맞춰 UAnimMontage* 와 bool 매개변수를 씁니다.
	// ★ 매우 중요: AddDynamic에 쓰이는 함수는 반드시 UFUNCTION()이 있어야 합니다.	
	UFUNCTION()
	void OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 모션워핑 타겟 업데이트 로직
	void SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition);

	// 업 도약 실행 및 모션 워핑 타겟 주입
	void HandleHopUp();
	// 업 도약 가능 여부 검사 
	bool CheckCanHopUp(FVector& OutHopUpTargetPosition);

	// 다운 도약 실행 및 모션 워핑 타겟 주입
	void HandleHopDown();
	// 다운 도약 가능 여부 검사 
	bool CheckCanHopDown(FVector& OutHopDownTargetPosition);

	// 다운 도약 실행 및 모션 워핑 타겟 주입
	void HandleHopLeft();
	// 다운 도약 가능 여부 검사 
	bool CheckCanHopLeft(FVector& OutHopLeftTargetPosition);

	// 다운 도약 실행 및 모션 워핑 타겟 주입
	void HandleHopRight();
	// 다운 도약 가능 여부 검사 
	bool CheckCanHopRight(FVector& OutHopRightTargetPosition);


#pragma endregion


#pragma region ClimCoreVariables

	TArray<FHitResult> ClimbableSurfacesTracedResults;
	
	// 다수 충돌 결과를 바탕으로 계산된 평균 표면 위치
	FVector CurrentClimbableSurfaceLocation;

	// 다수 충돌 결과를 바탕으로 계산된 평균 표면 방향
	FVector CurrentClimbableSurfaceNormal;

	// 몽타주를 재생할 애님 인스턴스 캐싱 변수
	UPROPERTY()
	UAnimInstance* OwningPlayerAnimInstance;

	UPROPERTY()
	ARunBoyRunCharacter* OwningPlayerCharacter;
	
	// 모션 워핑 컴포넌트 캐싱 * Transient > 일시적인 데이터 취급 
	UPROPERTY(BlueprintReadOnly, Category = "MotionWarping", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* CachedMotionWarpComp;

#pragma endregion


#pragma region ClimBPVariables
	
	// 검출할 오브젝트 타입(예: WorldStatic)을 담아둘 배열 ( 성능 최적화용 )
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	TArray<TEnumAsByte<EObjectTypeQuery>> ClimbableSurfaceTraceTypes;

	// 캡슐 트레이스의 반지름 (기본값 50.f)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceRadius = 50.f;

	// 캡슐 트레이스의 절반 높이 (기본값 72.f)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbCapsuleTraceHalfHeight = 72.f;

	// 클라이밍 중 키보드에서 손을 뗐을 때 멈추는 감속도 (마찰력을 대체함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxBreakClimbDeceleration = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxClimbSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 300.f;

	// 1차 트레이스 (바닥 확인)의 전방 오프셋 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbDownWalkableSurfaceTraceOffset = 100.f;
	
	// 2차 트레이스 (낭떠러지 확인)의 전방 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	float ClimbDownLedgeTraceOffset = 50.f;

	// 블루프린트에서 할당해 줄 몽타주 에셋 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleToClimbMontage;

	// 절벽 꼭대기에 도달했을 때 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbToTopMontage;

	// 절벽 하강 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbDownLedgeMontage;

	// 볼팅 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* VaultMontage;

	// 클라이밍 업 도약 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopUpMontage;

	// 클라이밍 다운 도약 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopDownMontage;

	// 클라이밍 좌측 도약 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopLeftMontage;

	// 클라이밍 우측 도약 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopRightMontage;

#pragma endregion

public:

	// 클라이밍 상태를 켜고 끄는 메인 토글 함수
	void ToggleClimbing(bool bEnableClimb);
	// 현재 클라이밍 중인지 확인하는 함수
	bool IsClimbing() const;

	// 도약 요청을 위한 방향 계산 알고리즘
	void RequestHopping();


	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }

	// 애니메이션에서 사용할 로컬 기준 속도 벡터 반환 함수 
	FVector GetUnrotatedClimbVelocity() const;
};

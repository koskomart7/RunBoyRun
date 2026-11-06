// Fill out your copyright notice in the Description page of Project Settings.


#include "RR_Components/CustomMovementComponent.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RunBoyRunCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MotionWarpingComponent.h"

#include "DebugHelper.h"

void UCustomMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    // 1. 캐릭터 오너의 스켈레탈 메시에서 애님 인스턴스를 가져와 캐싱
    OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();

    // 2. 델리게이트 바인딩
    if (OwningPlayerAnimInstance)
    {
        // 몽타주가 자연스럽게 끝났을 때
        OwningPlayerAnimInstance->OnMontageEnded.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
        // 몽타주가 블렌딩 아웃(다른 애니메이션으로 넘어가기 시작) 될 때
        OwningPlayerAnimInstance->OnMontageBlendingOut.AddDynamic(this, &UCustomMovementComponent::OnClimbMontageEnded);
    }

    OwningPlayerCharacter = Cast<ARunBoyRunCharacter>(CharacterOwner);

    // 모션 워핑 컴포넌트 캐싱 
    GetCachedMotionWarpingComponent();
}

UMotionWarpingComponent* UCustomMovementComponent::GetCachedMotionWarpingComponent()
{
    if (CachedMotionWarpComp)
    {
        return CachedMotionWarpComp;
    }

    if (OwningPlayerCharacter)
    {
        CachedMotionWarpComp = OwningPlayerCharacter->FindComponentByClass<UMotionWarpingComponent>();
    }

    return CachedMotionWarpComp;
}


void UCustomMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 매프레임 벽이 있는지 검사
    //TraceClimbableSurfaces();

    //TraceFromEyeHeight(100.f);

    //CanClimbDownLedge();
}

void UCustomMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
    // 1. 클라이밍 상태로 '진입'시 로직
    if (IsClimbing())
    {
        // 입력 방향으로 캐릭터가 자동 회전하는 기능 끄기 ( 벽을 바라보게 고정하기 위함 )
        bOrientRotationToMovement = false;
        // 캡슐의 절반높이를 절반으로 축소
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);
        
        // 등반 상태 진입 시 바인딩된 함수가 있다면 실행
        OnEnterClimbStateDelegate.ExecuteIfBound();
    }

    // 2. 클라이밍 상태에서 '탈출' 했을 때의 처리 ( 이전 모드가 Climb 이었는지 확인 )
    if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == ECustomMovementMode::MOVE_Climb)
    {
        // 자동 회전 기능 켜기
        bOrientRotationToMovement = true;

        // 캡슐 높이 복구 
        CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);

        // 클라이밍 종료시 회전 버그 수정 - 회전값 초기화
        const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
        // Pitch(끄덕임)와 Roll(기울어짐)을 0으로 깎고 Yaw(좌우 도리도리) 방향만 유지
        const FRotator CleamStandRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f);
        UpdatedComponent->SetRelativeRotation(CleamStandRotation);

        // 등반 중 발생한 잔여 가속도나 속력을 즉시 0으로 만들어 튕겨나감 방지
        StopMovementImmediately();

        // 등반 상태 종료 시 바인딩된 함수가 있다면 실행
        OnExitClimbStateDelegate.ExecuteIfBound();
    }

    // 3. 부모 클래스의 원래 로직 실행 
    Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UCustomMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
    // 현재 직접 만든 클라이밍 상태인지 확인
    if (IsClimbing())
    {
        PhysClimb(deltaTime, Iterations);
    }

    Super::PhysCustom(deltaTime, Iterations);
}

float UCustomMovementComponent::GetMaxSpeed() const
{
    if (IsClimbing())
    {
        return MaxClimbSpeed;
    }
    else
    {
        return Super::GetMaxSpeed();
    }
}

float UCustomMovementComponent::GetMaxAcceleration() const
{
    if (IsClimbing())
    {
        return MaxClimbAcceleration;
    }
    else
    {
        return Super::GetMaxAcceleration();
    }
}

FVector UCustomMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
    // 낙하(공중) 중이며, 애님 인스턴스가 유효하고, 어떤 몽타주라도 재생 중인지 확인
    const bool bIsPlayingRMMontage = 
        IsFalling() && OwningPlayerAnimInstance && OwningPlayerAnimInstance->IsAnyMontagePlaying();

    // 절벽 오르기 몽타주가 재생 중이라면 엔진의 Z축 무시 제약을 건너뛰고 온전한 루트 모션 속도를 그대로 반환
    if (bIsPlayingRMMontage)
    {
        return RootMotionVelocity;
    }
    else
    {
        // 그 외의 상황에서는 원래 엔진의 기본 로직을 따름
        return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
    }

}



#pragma region ClimbTraces

TArray<FHitResult> UCustomMovementComponent::DoCapsuleTraceMultiByObject(const FVector& Start,
    const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
    // 결과를 담을 빈 배열 생성
    TArray<FHitResult> OutCapsuleTraceHitResults;

    EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;
    
    if (bShowDebugShape)
    {
        DebugTraceType = EDrawDebugTrace::ForOneFrame;

        if (bDrawPersistantShapes)
        {
            DebugTraceType = EDrawDebugTrace::Persistent;
        }
    }
    
    UKismetSystemLibrary::CapsuleTraceMultiForObjects(
        this,                       // WorldContextObject (현재 컴포넌트)
        Start,                      // Trace 시작점
        End,                        // Trace 종료점
        ClimbCapsuleTraceRadius,    // 캡슐 반지름 (선언 변수)
        ClimbCapsuleTraceHalfHeight,// 캡슐 절반 높이 (선언 변수)
        ClimbableSurfaceTraceTypes, // 충돌을 검사할 오브젝트 타입 배열
        false,                      // 복잡한 충돌(Trace Complex) 사용 여부
        TArray<AActor*>(),          // 무시할 액터 배열 (여기선 빈배열)
        DebugTraceType,             // 디버그 드로잉 타입
        OutCapsuleTraceHitResults,  // 충돌 결과값이 담길 Out 매개변수
        false                       // 자기 자신 무시 여부
    );
    // 충돌 결과 배열 반환
    return OutCapsuleTraceHitResults;
}

FHitResult UCustomMovementComponent::DoLineTraceSingleByObject(const FVector& Start, 
    const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
    FHitResult OutHit;

    EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;
    if (bShowDebugShape)
    {
        DebugTraceType = EDrawDebugTrace::ForOneFrame;

        if (bDrawPersistantShapes)
        {
            DebugTraceType = EDrawDebugTrace::Persistent;
        }
    }

    UKismetSystemLibrary::LineTraceSingleForObjects(
        this,
        Start,
        End,
        ClimbableSurfaceTraceTypes,
        false,
        TArray<AActor*>(),
        DebugTraceType,
        OutHit,
        false
    );

    return OutHit;
}


#pragma endregion

#pragma region ClimbCore

void UCustomMovementComponent::ToggleClimbing(bool bEnableClimb)
{
    if (bEnableClimb)
    {
        // 클라이밍 시작조건 3가지 만족시 로직 진입
        if (CanStartClimbing())
        {
            // 클라이밍 상태 진입
            PlayClimbMontage(IdleToClimbMontage);
        }
        // 벽을 탈 수 없다면, 혹시 절벽 아래로 내려갈 수 있는지 확인 ( 하강 로직 )
        else if (CanClimbDownLedge())
        {
            // 하강 몽타주
            PlayClimbMontage(ClimbDownLedgeMontage);
        }
        else
        {
            // 클라이밍이나 하강이 모두 불가능한 환경일 경우 Vaulting 시도 
            TryStartVaulting();
        }
    }

    if(!bEnableClimb)
    {
        // 클라이밍 중단
        StopClimbing();
    }
}

bool UCustomMovementComponent::CanStartClimbing()
{
    // 공중에 떠 있는 상태면 불가
    if (IsFalling()) return false;
    // 전방 캡슐트레이스 결과가 없으면 등반불가 (벽이없다면)
    if (!TraceClimbableSurfaces()) return false;
    // 시선 높이 트레이스 결과 벽이 없으면 불가
    if (!TraceFromEyeHeight(100.f).bBlockingHit) return false;

    return true;
}

bool UCustomMovementComponent::CanClimbDownLedge()
{
    if (IsFalling()) return false;

    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector ComponentForward = UpdatedComponent->GetForwardVector();
    const FVector DownVector = -UpdatedComponent->GetUpVector();

    /* 1. 첫 번째 트레이스: 캐릭터 전방의 바닥(Walkable Surface) 확인 */
    const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward * ClimbDownWalkableSurfaceTraceOffset;
    const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

    FHitResult WalkableSurfaceHit =
        DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);

    /* 2. 두 번째 트레이스: 그 너머의 낭떠러지(Ledge) 허공 확인 */
    // 1차 트레이스 시작점에서 앞쪽으로 오프셋만큼 한 번 더 이동
    const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
    const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 200.f;

    FHitResult LedgeTraceHit = DoLineTraceSingleByObject(LedgeTraceStart, LedgeTraceEnd);

    /* 3. 최종 조건 검사 */
    // 바닥에는 발을 디딜 수 있고(True) && 전방은 낭떠러지로 뚫려 있어야 함(False)
    if (WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit)
    {
        return true;
    }

    return false;
}

void UCustomMovementComponent::StartClimbing()
{
    // 무브먼트 모드를 커스텀으로 변경, 서브모드를 직접 만든 Climb로 설정
    SetMovementMode(MOVE_Custom, ECustomMovementMode::MOVE_Climb);
}

void UCustomMovementComponent::StopClimbing()
{
    // 클라이밍을 취소할 때는 공중으로 떨어지도록 Falling 모드로 변경
    SetMovementMode(MOVE_Falling);
}

void UCustomMovementComponent::PhysClimb(float deltaTime, int32 Iterations)
{
    if (deltaTime < MIN_TICK_TIME)
    {
        return;
    }

    /* 1. 표면 정보 처리 */
    // 매 프레임 전방으로 캡슐 트레이스를 쏘아 배열을 갱신
    TraceClimbableSurfaces();

    // 갱신된 배열을 바탕으로 평균 위치/노멀 계산
    ProcessClimableSurfaceInfo();


    /* 2. 등반 종료 조건 검사 (Check if we should stop climbing)*/
    if (CheckShouldStopClimbing() || CheckHasReachedFloor())
    {
        StopClimbing();
    }

    // 루트 모션 복구
    // 이전 프레임에 애니메이션 루트 모션이 속도에 추가(Additive) 되기 전의 원래 속도를 복원하는 함수
    // 물리 계산은 “루트 모션이 더해지기 전의 속도”를 기준으로 해야 하기 때문
    RestorePreAdditiveRootMotionVelocity();

    // 루트 모션이 없는 경우에만 속도 계산
    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {	
        /* 3. 등반 최고 속도 및 가속도 정의 (Define max climb speed and acceleration) */

        // 속도 계산 (마찰력을 0으로 두고, 직접 만든 감속도 변수를 대입)
        CalcVelocity(deltaTime, 0.f, true, MaxBreakClimbDeceleration);
    }

    // 루트 모션을 현재 속도에 적용
    ApplyRootMotionToVelocity(deltaTime);

    // 실제 캐릭터를 이동시키는 핵심 엔진 함수
    FVector OldLocation = UpdatedComponent->GetComponentLocation(); // 이동전 위치 : 캐릭터의 충돌 캡슐 위치
    const FVector Adjusted = Velocity * deltaTime;  // 이번 틱 동안 이동해야할 거리
    FHitResult Hit(1.f);    // 충돌 정보 구조체

    /* 4. 등반 중 회전 처리 (Handle climb rotation) */
    SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

    if (Hit.Time < 1.f)     // 중간에 뭔가에 부딪혔다는 뜻
    {
        // 이동 중 충돌 발생 시 표면을 따라 미끄러지도록 처리
        HandleImpact(Hit, deltaTime, Adjusted);
        SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
    }

    // 최종 Veolocity 재계산
    if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
    {
        Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
    }

    /* 5. 표면에 스냅 처리 (Snap movement to climbable surfaces) */
    SnapMovementToClimableSurfaces(deltaTime);

    // 절벽의 꼭대기 도달 감지 함수
    if (CheckHasReachedLedge())
    {
       // 애니메이션의 루트 모션이 캐릭터를 위로 당겨준다
       PlayClimbMontage(ClimbToTopMontage);
    }
   
 
}

void UCustomMovementComponent::ProcessClimableSurfaceInfo()
{
    // 1. 변수 초기화 (매 프레임 이전 쓰레기값이 남지 않도록 0으로 덮어씀)
    CurrentClimbableSurfaceLocation = FVector::ZeroVector;
    CurrentClimbableSurfaceNormal = FVector::ZeroVector;

    // 2. 만약 트레이스 결과가 없다면 조기 종료(안전장치)
    if (ClimbableSurfacesTracedResults.IsEmpty()) return;

    // 3. 배열을 순회하며 위치와 노멀 값을 전부 더함
    for (const FHitResult& TracedHitResult : ClimbableSurfacesTracedResults)
    {
        CurrentClimbableSurfaceLocation += TracedHitResult.ImpactPoint;
        CurrentClimbableSurfaceNormal += TracedHitResult.ImpactNormal;
    }

    // 4. 합산된 위치를 충돌한 횟수(배열 길이)로 나누어 '평균 위치' 산출
    CurrentClimbableSurfaceLocation /= ClimbableSurfacesTracedResults.Num();
    
    // 5. 합산된 노멀을 정규화하여 길이가 1인 '평균 단위 벡터'로 변환
    CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();

    //Debug::Print(TEXT("ClimbableSurfaceLocation : ") + CurrentClimbableSurfaceLocation.ToCompactString(), FColor::Cyan, 1);
    //Debug::Print(TEXT("ClimbableSurfaceNormal: ") + CurrentClimbableSurfaceNormal.ToCompactString(), FColor::Red, 2);
}

bool UCustomMovementComponent::CheckShouldStopClimbing()
{
    // 안전 조기 종료
    if (ClimbableSurfacesTracedResults.IsEmpty()) return true;

    // 1. 표면 노멀과 하늘 방향(UpVector)의 내적
    const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
    // 2. 내적 값에 아크코사인을 적용해서 라디안 각도를 구함 (역코사인 함수 적용)
    // 3. 구한 라디안을 Degree(도)로 변환
    const float DegreeDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));
    
    // 4. 표면 노멀과 하늘 방향의 각도 차이가 60도 이하면 등반 종료
    // 60도 대신 레벨의 다양한 액터의 표면 정보를 변수로 사용가능
    if (DegreeDiff <= 60.f)
    {
        return true;
    }

    // Debug::Print(TEXT("Degree Diff : ") + FString::SanitizeFloat(DegreeDiff), FColor::Cyan, 1);
    
    // 그외의 조건에서는 계속 등방 유지
    return false;
}

bool UCustomMovementComponent::CheckHasReachedFloor()
{
    // 1. 아래를 향하는 벡터 생성
    const FVector DownVector = -UpdatedComponent->GetUpVector();
    
    // 2. 시작점을 현재 위치에서 약간 아래로 이동 
    const FVector StartOffset = DownVector * 50.f;
    const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;

    // 3. 끝점(End)은 시작점에서 아주 미세하게 아래로 설정 (캡슐 형태 유지)
    const FVector End = Start + DownVector;

    // 4. 하향 멀티 캡슐 트레이스 실행
    TArray<FHitResult> PossibleFloorHits = DoCapsuleTraceMultiByObject(Start, End);

    // 충돌 결과가 없으면(공중이면) false 반환
    if (PossibleFloorHits.IsEmpty()) return false;

    // 5. 충돌 결과 배열을 순회하며 진짜 바닥인지 검사
    for (const FHitResult& PossibleFloorHit : PossibleFloorHits)
    {
        const bool bFloorReached = 
            // 바닥 판별 조건 1: 표면 노멀의 역방향이 월드 Up 벡터와 평행(Parallel)한지 확인
            FVector::Parallel(-PossibleFloorHit.ImpactNormal, FVector::UpVector) && 
            // 바닥 판별 조건 2: 캐릭터가 실제로 아래로 내려가고 있는 중인지 (속도 Z < -10)
            GetUnrotatedClimbVelocity().Z < -10.f;

        // 두 조건을 모두 만족하면 바닥에 성공적으로 착지한 것으로 간주
        if (bFloorReached)
        {
            return true;
        }
    }

    return false;
}

FQuat UCustomMovementComponent::GetClimbRotation(float DeltaTime)
{
    const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();

    // 루트 모션(애니메이션)이 회전을 제어 중일 때는 코드로 강제 회전시키지 않음
    if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
    {
        return CurrentQuat;
    }

    // 1. 벽의 노멀 벡터를 반대로 뒤집어 캐릭터가 바라보아야 할 정면(X축) 벡터로 사용
    // 2. MakeFromX로 Rotator를 만든 뒤 Quaternion으로 변환
    const FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();

    // 3. 현재 회전에서 목표 회전으로 QInterpTo를 사용해 부드럽게 보간 (Speed: 5.f)
    return FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 5.f);
}

void UCustomMovementComponent::SnapMovementToClimableSurfaces(float DeltaTime)
{
    const FVector ComponentForward = UpdatedComponent->GetForwardVector();
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();

    // 1. 캐릭터에서 표면으로 향하는 원본 벡터
    const FVector ProjectedCharacterToSurface = 
        (CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentForward);

    // 2. 투영된 벡터의 길이(거리)만큼 벽의 반대 방향으로 밀착시키는 벡터 계산
    const FVector SnapVector = -CurrentClimbableSurfaceNormal * ProjectedCharacterToSurface.Length();

    // 3. MoveComponent를 사용하여 실제로 캐릭터를 밀착시킴
    // Delta = SnapVector * DeltaTime * MaxClimbSpeed (부드러운 이동)
    UpdatedComponent->MoveComponent(
        SnapVector * DeltaTime * MaxClimbSpeed,
        UpdatedComponent->GetComponentQuat(), // 앞서 만든 회전 함수 적용
        true);                                // bSweep = true (이동 중 충돌 감지 켜기)
}

bool UCustomMovementComponent::CheckHasReachedLedge()
{
    // 1.  첫 번째 트레이스 (머리 위쪽 공간 탐색)
    FHitResult LedgetHitResult = TraceFromEyeHeight(100.f, 50.f);

    // 2. 만약 첫 번째 트레이스가 어떤 벽도 맞추지 않았다면 (위가 뚫려 있다면)
    if (!LedgetHitResult.bBlockingHit)
    {
        // 3. 두 번째 트레이스의 시작점 : 1차 트레이스의 허공 끝점
        const FVector WalkableSurfaceTraceStart = LedgetHitResult.TraceEnd;

        // 4. 두 번째 트레이스의 끝점 : 시작점에서 아래로 100.f 만큼 내린 곳
        const FVector DownVector = -UpdatedComponent->GetUpVector();
        const FVector WalkaBledSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

        // 5. 하향 라인 트레이스 발사 ( 단일 객체 추적 이용 )
        FHitResult WalkableSurfaceHitResult = 
            DoLineTraceSingleByObject(WalkableSurfaceTraceStart, WalkaBledSurfaceTraceEnd);

        // 6. 만약 하향 트레이스가 바닥을 감지 했고 && 캐릭터가 위로 올라가고 있는 중 (Z > 10.f ) 이라면
        if (WalkableSurfaceHitResult.bBlockingHit && GetUnrotatedClimbVelocity().Z > 10.f)
        {
            return true;    // 절벽 모서리 감지 성공
        }
    }

    return false;
}

void UCustomMovementComponent::TryStartVaulting()
{
    FVector VaultStartPosition;
    FVector VaultLandPosition;

    if (CanStartvaulting(VaultStartPosition, VaultLandPosition))
    {
        SetMotionWarpTarget(FName("VaultStartPoint"), VaultStartPosition);
        SetMotionWarpTarget(FName("VaultEndPoint"), VaultLandPosition);

        StartClimbing();
        PlayClimbMontage(VaultMontage);
    }

}

bool UCustomMovementComponent::CanStartvaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition)
{
    // 1. 상태 체크: 공중에 떠 있는 상태(점프 중 등)라면 볼팅을 시작할 수 없음
    if (IsFalling()) return false;

    // 시작 지점과 착지 지점 변수를 초기화 ( 0,0,0 )
    OutVaultStartPosition = FVector::ZeroVector;
    OutVaultLandPosition = FVector::ZeroVector;

    // 2. 캐릭터의 현재 위치 및 방향 데이터 가져오기 
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    const FVector ComponentForward = UpdatedComponent->GetForwardVector();
    const FVector UpVector = UpdatedComponent->GetUpVector();
    const FVector DownVector = -UpdatedComponent->GetUpVector();

    // 3. 루프를 돌며 캐릭터 앞쪽 지형 탐색 ( 총 5번 검사 )
    for (int32 i = 0; i < 5; i++)
    {
        // 시작점 계산 : 캐릭터 위치에서 위로 100 만큼 띄운 뒤, 앞쪽으로 설정 변수 만큼 전진
        const FVector Start = ComponentLocation + UpVector * 100.f + ComponentForward * 80.f * (i + 1);
        
        // 끝점 계산 : 시작점에서 아래 방향으로 ( 100, 200, 300, 400, 500 )만큼 빔을 쏜다
        // i 가 커질수록 더 앞쪽을 검사하고, 더 깊게 아래를 확인한다
        const FVector End = Start + DownVector * 100.f * (i + 1);

        // 실제로 빔을 쏴서 물체가 있는지 확인
        FHitResult VaultTraceHit = DoLineTraceSingleByObject(Start, End);

        // 4. 첫번째 검사 ( 가장 가까운 곳 )에서 충돌이 발생하면 Vaulting 시작 지접으로 설정
        if (i == 0 && VaultTraceHit.bBlockingHit)
        {
            OutVaultStartPosition = VaultTraceHit.ImpactPoint;
        }

        // 5. 다섯 번째 검사 ( 가장 먼 곳 )에서 충돌이 발생하면 볼팅 착지 지점으로 설정
        if (i == 3 && VaultTraceHit.bBlockingHit)
        {
            OutVaultLandPosition = VaultTraceHit.ImpactPoint;
        }
    }

    // 6. 최종 판단 : 시작점과 착지점 두 곳 모두 유효한 지면이 발견되었다면 true 반환 
    if (OutVaultStartPosition != FVector::ZeroVector && OutVaultLandPosition != FVector::ZeroVector)
    {
        return true;
    }
    else
    {
        return false;
    }

}



bool UCustomMovementComponent::IsClimbing() const
{
    // 기본 모드가 Custom이고, 커스텀 모드가 직접 만든 Climb 모드일 때만 true 반환
    return MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::MOVE_Climb;
}




// 클라이밍 가능한 표면을 추적합니다. 
// 유효한 표면이 존재할 경우 true를 반환하고, 그렇지 않을 경우 false를 반환합니다.
bool UCustomMovementComponent::TraceClimbableSurfaces()
{
    // 캐릭터의 전방 방향 벡터에서 30만큼 떨어진 곳
    const FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.f;
    // 캐릭터의 현재 위치를 UpdatedComponent를 통해 가져온다
    const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
    // Start에서 앞으로 아주 미세하게(크기 1) 이동한 곳 (캡슐이 제자리에 머물도록 유도)
    const FVector End = Start + UpdatedComponent->GetForwardVector();

    // 다중 캡슐 트레이스 함수 호출
    ClimbableSurfacesTracedResults = DoCapsuleTraceMultiByObject(Start, End);

    return !ClimbableSurfacesTracedResults.IsEmpty();
}

//  눈 높이의 라인트레이스 추적 
FHitResult UCustomMovementComponent::TraceFromEyeHeight(
    float TraceDistance, float TraceStartOffset, 
    ETraceDirectionAxis AxisType,
    bool bShowDebugShape, bool bDrawPersistantShapes,
    FVector CustomDirection
)
{
   /*  to do : 현재 로직은 대각선 방향으로 도약하는 로직임. 
               추후 수평으로 도약할수있게 별도의 Trace 함수를 만들거나 매개변수 수정 */
    FVector FinalAxis;

    // 축 선택 로직
    switch (AxisType)
    {
    case ETraceDirectionAxis::Up:
        FinalAxis = UpdatedComponent->GetUpVector();  // 기본값
        break;
    case ETraceDirectionAxis::Right:
        FinalAxis = UpdatedComponent->GetRightVector();
        break;
    case ETraceDirectionAxis::Forward:
        FinalAxis = UpdatedComponent->GetForwardVector();
        break;
    case ETraceDirectionAxis::Custom:
        FinalAxis = CustomDirection.IsNearlyZero() ? UpdatedComponent->GetUpVector() : CustomDirection.GetSafeNormal();
        break;
    }
    
    const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
    // Z축(위쪽) 방향 벡터를 구한 뒤, BaseEyeHeight와 추가 오프셋을 더한 만큼 곱해줌
    const FVector EyeHeightOffset = FinalAxis * (CharacterOwner->BaseEyeHeight + TraceStartOffset);

    const FVector Start = ComponentLocation + EyeHeightOffset;
    const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

   return DoLineTraceSingleByObject(Start, End, bShowDebugShape, bDrawPersistantShapes);
}


void UCustomMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
    // 안전장치
    // 재생할 몽타주 에셋이 비어있으면, 애님 인스턴스가 없거나, 이미 다른 몽타주 재생시 취소
    if (!MontageToPlay) return;
    if (!OwningPlayerAnimInstance) return;
    if (OwningPlayerAnimInstance->IsAnyMontagePlaying()) return;
    
    // 모든 조건이 완벽하면 몽타주 재생 명령 하달
    OwningPlayerAnimInstance->Montage_Play(MontageToPlay);
}

// 몽타주 재생이 완료되면 엔진이 자동으로 호출해주는 함수
void UCustomMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 방금 재생이 끝난 몽타주가 'IdleToClimbMontage'인지 확인
    if (Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
    {
        // 몽타주가 끝나면 실제 클라이밍 물리 상태 진입
        StartClimbing();
        StopMovementImmediately();
    }
    
    if(Montage == ClimbToTopMontage || Montage == VaultMontage)
    {
        SetMovementMode(MOVE_Walking);

        FString ModeName = StaticEnum<EMovementMode>()->GetValueAsString(MovementMode);
        //Debug::Print(TEXT("Now SetMovementMode : ") + ModeName, FColor::Cyan, 5);

    }
}


void UCustomMovementComponent::RequestHopping()
{
    // 1. 마지막으로 입력된 방향 벡터 가져오기
    // 이 벡터는 월드 공간 기준이며 표면의 회전값에 영향을 받는다
    FVector LastInputVector = GetLastInputVector();

    // 2. 월드 공간 벡터를 로컬(표면) 공간으로 Unrotate 및 정규화
    // 캐릭터 컴포넌트의 쿼터니언(Quat)을 기준으로 역회전시켜 일관된 상하좌우 방향을 얻습니다.
    // LessLess_VectorRotator 사용 가능 -> FRotator (Pitch, Yaw, Roll)를 입력 받음
    const FVector UnrotatedLastInputVector = UKismetMathLibrary::Quat_UnrotateVector(
        UpdatedComponent->GetComponentQuat(), LastInputVector);

    // 3. 내적(Dot Product)을 활용한 방향 판별
    // 정규화된 입력 벡터와 월드의 순수 상단 벡터(FVector::UpVector)를 비교합니다.
    const float DotUp = FVector::DotProduct(
        UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);
    
    const float DotRight = FVector::DotProduct(
        UnrotatedLastInputVector.GetSafeNormal(), FVector::RightVector);

    // Debug::Print(TEXT("Dot result: ") + FString::SanitizeFloat(DotResult));
    
    // 4. 내적 결과값에 따른 조건 분기
    if (DotUp >= 0.9f)
    {
        // 입력 방향과 위쪽 방향이 일치함 (Hop Up)
        //Debug::Print(TEXT("Hop Up"));
        HandleHopUp();

    }
    else if (DotUp <= -0.9f)
    {
        // 입력 방향과 위쪽 방향이 정반대임 (Hop Down)
        //Debug::Print(TEXT("Hop Down"));
        HandleHopDown();
    }
    else if (DotRight >= 0.9f)
    {
       
        //Debug::Print(TEXT("Hop Right"));
        HandleHopRight();
    }
    else if (DotRight <= -0.9f)
    {
        //Debug::Print(TEXT("Hop Left"));
        HandleHopLeft();
    }
    else
    {
        //Debug::Print(TEXT("Invalid Input Range"));
    }
}

void UCustomMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition)
{
    if (!OwningPlayerCharacter) return;
    
    UMotionWarpingComponent* MotionWarpComp = GetCachedMotionWarpingComponent();

    // 캐싱된 컴포넌트 Null 체크
    if (MotionWarpComp)
    {
        // 디버깅을 위해 위치가 제대로 넘어오는지 로그 출력
        //Debug::Print(FString::Printf(TEXT("Warping to: %s"), *InTargetPosition.ToString()), FColor::Yellow);
       
        MotionWarpComp->AddOrUpdateWarpTargetFromLocation(InWarpTargetName, InTargetPosition);
    }
    else
    {
        //Debug::Print(TEXT("SetMotionWarpTarget Failed: UMotionWarpingComponent reference is NULL. Check Character Constructor or Build.cs."));
        return;
    }
       

   
   /* OwningPlayerCharacter->GetMotionWarpingComp()->AddOrUpdateWarpTargetFromLocation(
        InWarpTargetName, 
        InTargetPosition
    );*/
}

void UCustomMovementComponent::HandleHopUp()
{
    FVector HopUpTargetPoint;

    // 지형 검사 통과 시
    if (CheckCanHopUp(HopUpTargetPoint))
    {
        // 1. 모션 워핑 타겟 이름과 좌표 세팅 (몽타주 노티파이의 설정 이름과 동일해야 함) 
        SetMotionWarpTarget(FName("HopUpTargetPoint"), HopUpTargetPoint);

        // 2. 위로 도약하는 몽타주 재생
        PlayClimbMontage(HopUpMontage);
    }
}

bool UCustomMovementComponent::CheckCanHopUp(FVector& OutHopUpTargetPosition)
{
    // 1. 첫 번째 트레이스 ( 가슴 높이 ) : 손을 짚을 벽면 검사 ( 거리 , 눈 높이 기준 탐색시작 지점 )
    FHitResult HopUpHit = TraceFromEyeHeight(100.f, -20.f);

    // 2. 두 번째 트레이스 ( 머리 위 ) : Ledge 도달 여부 검사 ( 안전 장치 ) - 위쪽 공감 여유 검사 
    FHitResult SafetyLedgeHit = TraceFromEyeHeight(100.f, 150.f);

    // 두 트레이스 모두 유효 할 때 도약 가능 true 값 반환
    if (HopUpHit.bBlockingHit && SafetyLedgeHit.bBlockingHit)
    {
        // 모션 워핑의 목적지로 사용될 충돌 지점 저장
        OutHopUpTargetPosition = HopUpHit.ImpactPoint;

        return true;
    }

    return false;
}

void UCustomMovementComponent::HandleHopDown()
{
    FVector HopDownTargetPoint;

    if (CheckCanHopDown(HopDownTargetPoint))
    {
        SetMotionWarpTarget(FName("HopDownTargetPoint"), HopDownTargetPoint);

        PlayClimbMontage(HopDownMontage);
    }
}

bool UCustomMovementComponent::CheckCanHopDown(FVector& OutHopDownTargetPosition)
{
    // 캐릭터 눈 높이에서 300 유닛(-300.f)만큼 내려간 지점부터 트레이스 시작
    FHitResult HopDownHit = TraceFromEyeHeight(100.f, -300.f);

    if (HopDownHit.bBlockingHit)
    {
        OutHopDownTargetPosition = HopDownHit.ImpactPoint;
        
        return true;
    }

    return false;
}

void UCustomMovementComponent::HandleHopLeft()
{
    FVector HopLeftTargetPoint;
    if (CheckCanHopLeft(HopLeftTargetPoint))
    {
        // 몽타주 노티파이의 Warp Target Name과 반드시 일치해야 함
        SetMotionWarpTarget(TEXT("HopLeftTargetPoint"), HopLeftTargetPoint);
        PlayClimbMontage(HopLeftMontage);
    }
}

bool UCustomMovementComponent::CheckCanHopLeft(FVector& OutHopLeftTargetPosition)
{
    FHitResult HopLeftHit = TraceFromEyeHeight(100.f, -300.f, ETraceDirectionAxis::Right, true, true);

    if (HopLeftHit.bBlockingHit)
    {
        OutHopLeftTargetPosition = HopLeftHit.ImpactPoint;

        return true;
    }

    return false;
}

void UCustomMovementComponent::HandleHopRight()
{
    FVector HopRightTargetPoint;
    if (CheckCanHopRight(HopRightTargetPoint))
    {
        SetMotionWarpTarget(TEXT("HopRightTargetPoint"), HopRightTargetPoint);
        PlayClimbMontage(HopRightMontage);
    }
}

bool UCustomMovementComponent::CheckCanHopRight(FVector& OutHopRightTargetPosition)
{
    FHitResult HopRightHit = TraceFromEyeHeight(100.f, 200.f, ETraceDirectionAxis::Right,true, true);

    if (HopRightHit.bBlockingHit)
    {
        OutHopRightTargetPosition = HopRightHit.ImpactPoint;

        return true;
    }

    return false;
}

FVector UCustomMovementComponent::GetUnrotatedClimbVelocity() const
{
    // 현재 캐릭터의 회전(Quaternion)을 기준으로 월드 Velocity를 역회전하여 반환 (로컬기준의 정보) 
    return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(),Velocity);
}

#pragma endregion



// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/CharacterAnimInstance.h"
#include "RunBoyRunCharacter.h"
#include "RR_Components/CustomMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RBRCharacter = Cast<ARunBoyRunCharacter>(TryGetPawnOwner());

	if (RBRCharacter)
	{
		CustomMovementComponent = RBRCharacter->GetCustomMovementComponent();
	}
}

void UCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!RBRCharacter || !CustomMovementComponent) return;

	GetGroundSpeed();
	GetAirSpeed();
	GetShouldMove();
	GetIsFalling();
	GetIsClimbing();
	GetClimbVelocity();
}

void UCharacterAnimInstance::GetGroundSpeed()
{
	// Z축을 제외한 X, Y 평면상의 속도만 추출
	GroundSpeed = UKismetMathLibrary::VSizeXY(RBRCharacter->GetVelocity());
}

void UCharacterAnimInstance::GetAirSpeed()
{
	// Z축 속도만 추출 (상승/하강 속도)
	AirSpeed = RBRCharacter->GetVelocity().Z;
}

void UCharacterAnimInstance::GetShouldMove()
{
	// 가속도(입력)가 존재하고, 지상 속도가 5 이상이며, 공중에 떠있지 않을 때만 이동 애니메이션 재생
	bShouldMove = 
		CustomMovementComponent->GetCurrentAcceleration().Size() > 0 &&
		GroundSpeed > 5.f &&
		!bIsFalling;
}

void UCharacterAnimInstance::GetIsFalling()
{
	bIsFalling = CustomMovementComponent->IsFalling();
}

void UCharacterAnimInstance::GetIsClimbing()
{
	// 무브먼트 컴포넌트의 IsClimbing() 결과(bool)를 가져와 저장
	bIsClimbing = CustomMovementComponent->IsClimbing();
}

void UCharacterAnimInstance::GetClimbVelocity()
{
	// 무브먼트 컴포넌트에서 계산한 로컬 속도를 가져와 저장
	ClimbVelocity = CustomMovementComponent->GetUnrotatedClimbVelocity();
}

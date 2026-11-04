// Copyright Epic Games, Inc. All Rights Reserved.

#include "RunBoyRunCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
//#include "GameFramework/CharacterMovementComponent.h"
#include "RR_Components/CustomMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "RunBoyRun.h"
#include "MotionWarpingComponent.h"
#include "DebugHelper.h"


ARunBoyRunCharacter::ARunBoyRunCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 커스텀 무브먼트 캐싱 - 형변환 비용 줄이는 목적 
	CustomMovementComponent = Cast<UCustomMovementComponent>(GetCharacterMovement());
	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;	// 캐릭터가 입력 방향으로 움직일수 있다 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 참고: 메쉬 컴포넌트(캐릭터에서 상속됨)의 스켈레탈 메쉬 및 애니메이션 블루프린트 참조 
	// 파생된 블루프린트 애셋인 ThirdPersonCharacter에 설정됩니다(C++에서 직접적인 콘텐츠 참조를 피하기 위해).
	
	// 모션 워핑 컴포넌트 생성
	MotionWarpingComp = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));


}

void ARunBoyRunCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 모션워핑 유효성 검사로직
	/*TArray<UMotionWarpingComponent*> MWComps;
	GetComponents(MWComps);
	
	UE_LOG(LogRunBoyRun, Warning, TEXT("[MW-Diag] %s | MotionWarpingComp=%s | MW component count=%d"),
	*GetName(), *GetNameSafe(MotionWarpingComp), MWComps.Num());
	
	for (const UMotionWarpingComponent* Comp : MWComps)
	{
		UE_LOG(LogRunBoyRun, Warning, TEXT("[MW-Diag]   - %s (Archetype: %s)"),
		*GetNameSafe(Comp), *GetNameSafe(Comp->GetArchetype()));
	}*/
	
	
	// 기본조작 ( 항상 켜저있는 입력 )은 컨트롤러에서 관리하므로 여기에 선언 필요 X 
	// AddInputMappingContext(DefaultMappingContext,0);

	if (CustomMovementComponent)
	{
		// 델리게이트에 캐릭터의 콜백 함수 연결
		CustomMovementComponent->OnEnterClimbStateDelegate.BindUObject(this, &ThisClass::OnPlayerEnterClimbState);
		CustomMovementComponent->OnExitClimbStateDelegate.BindUObject(this, &ThisClass::OnPlayerExitClimbState);
	}
}

void ARunBoyRunCharacter::AddInputMappingContext(UInputMappingContext* ContextToAdd, int32 InPriority)
{
	if (!ContextToAdd) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// 지정된 우선순위로 컨텍스트 추가 
			Subsystem->AddMappingContext(ContextToAdd, InPriority);
		}
	}
}

void ARunBoyRunCharacter::RemoveInputMappingContext(UInputMappingContext* ContextToRemove)
{
	if (!ContextToRemove) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// 컨텍스트 제거 
			Subsystem->RemoveMappingContext(ContextToRemove);
		}
	}
}

void ARunBoyRunCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARunBoyRunCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ARunBoyRunCharacter::Look);

		// 클라이밍 매핑 컨택스트 
		EnhancedInputComponent->BindAction(ClimbMoveAction, ETriggerEvent::Triggered, this, &ARunBoyRunCharacter::HandleClimbMovementInput);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARunBoyRunCharacter::Look);
		
		// 클라이밍 액션
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &ARunBoyRunCharacter::OnClimbActionStarted);

		// 클라이밍 도약 액션
		EnhancedInputComponent->BindAction(ClimbHopAction, ETriggerEvent::Started, this, &ARunBoyRunCharacter::OnClimbHopActionStarted);
	
	}
	else
	{
		UE_LOG(LogRunBoyRun, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ARunBoyRunCharacter::Move(const FInputActionValue& Value)
{
	if (!CustomMovementComponent || !bCanMove) return;

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// HandleClimbMovementInput을 인풋에 바인딩하여 상태 체크할 필요가 없다.
	/*if (CustomMovementComponent->IsClimbing())
	{
		HandleClimbMovementInput(Value);
		return;
	}*/

	if (bIsOnBeam)
	{
		// 빔 위에 있을 때 : BP 오버라이드 함수 호출
		MoveAlongBeam();
		return;
	}

	if (bIsCover)
	{
		// 은엄폐 커버 상태일 때 : BP 오버라이드 함수 호출
		InputMoving();
		return;
	}

	// WallSliding 제어 변수
	if (bIsWallSliding)
	{
		return;
	}
	// 일반 지면일 때: 기존 이동 함수 호출
	DoMove(MovementVector.X, MovementVector.Y);
	
	//HandleGroundMovementInput(Value);
}

//void ARunBoyRunCharacter::HandleGroundMovementInput(const FInputActionValue& Value)
//{
//	// input is a Vector2D
//	const FVector2D MovementVector = Value.Get<FVector2D>();
//
//	if ((GetController() != nullptr) && bCanMove)
//	{
//		// find out which way is forward
//		const FRotator Rotation = GetController()->GetControlRotation();
//		const FRotator YawRotation(0, Rotation.Yaw, 0);
//
//		// get forward vector
//		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
//
//		// get right vector 
//		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
//
//		// add movement 
//		AddMovementInput(ForwardDirection, MovementVector.Y);
//		AddMovementInput(RightDirection, MovementVector.X);
//	}
//}

void ARunBoyRunCharacter::HandleClimbMovementInput(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// 1. 상하 (Forward) 이동 방향 계산 (외적)-언리얼만의 좌수법칙 적용
	const FVector ForwardDirection = FVector::CrossProduct(
		-CustomMovementComponent->GetClimbableSurfaceNormal(), 
		GetActorRightVector()
	);

	// 2. 좌우 (Right) 이동 방향 계산 (외적)
	const FVector RightDirection = FVector::CrossProduct(
		-CustomMovementComponent->GetClimbableSurfaceNormal(),
		-GetActorUpVector()
	);

	// 계산된 새 축을 바탕으로 캐릭터에 입력값 추가 
	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ARunBoyRunCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}


void ARunBoyRunCharacter::DoMove(float Right, float Forward)
{
	if ((GetController() != nullptr) && bCanMove)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);  // ( y, z, x )

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// WallRun 조건 검사 방향 벡터 추가
		const FVector ModifiedRightDirection = bIsWallRunning ? GetActorForwardVector() : RightDirection;

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(ModifiedRightDirection, Right);
	}
}

void ARunBoyRunCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ARunBoyRunCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ARunBoyRunCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}


void ARunBoyRunCharacter::MoveAlongBeam_Implementation()
{
	// 블루프린트에서 구현
}

void ARunBoyRunCharacter::InputMoving_Implementation()
{
	// 블루프린트에서 구현
}

void ARunBoyRunCharacter::OnClimbActionStarted(const FInputActionValue& Value)
{
	// 컴포넌트 유효성 검사 조기종료
	if (!CustomMovementComponent) return;
	
	// 현재 클라이밍 중이 아니면 True , 클라이밍 중이면 False
	if (!CustomMovementComponent->IsClimbing())
	{
		CustomMovementComponent->ToggleClimbing(true);
	}
	else
	{
		CustomMovementComponent->ToggleClimbing(false);
	}

}


void ARunBoyRunCharacter::OnPlayerEnterClimbState()
{
	// 등반 시작 시: 우선순위 1로 ClimbMappingContext 추가 (기본 걷기 이동 덮어쓰기)
	AddInputMappingContext(ClimbMappingContext, 1);

}

void ARunBoyRunCharacter::OnPlayerExitClimbState()
{
	// 등반 종료 시 : ClimbMappingContext 제거(다시 기본 걷기 모드로 복귀)
	RemoveInputMappingContext(ClimbMappingContext);

	
}


void ARunBoyRunCharacter::OnClimbHopActionStarted(const FInputActionValue& Value)
{
	if (CustomMovementComponent)
	{
		CustomMovementComponent->RequestHopping();
	}
}
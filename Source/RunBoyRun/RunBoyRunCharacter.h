// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"

#include "RunBoyRunCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UCustomMovementComponent;
class UMotionWarpingComponent;
class UInputMappingContext;
struct FInputActionValue;



DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ARunBoyRunCharacter : public ACharacter
{
	GENERATED_BODY()


public:

	/** Constructor  - 커스텀 무브먼트 컴포넌트 추가를위한 매개변수 추가 */	
	ARunBoyRunCharacter(const FObjectInitializer& ObjectInitializer);

private:

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	UCustomMovementComponent* CustomMovementComponent;
	
protected:
#pragma region Input


	// 클라이밍 상태 델리게이트 실행시 호출될 함수
	void OnPlayerEnterClimbState();
	void OnPlayerExitClimbState();

	// 입력 인풋 헬퍼 함수 
	void AddInputMappingContext(UInputMappingContext* ContextToAdd, int32 InPriority);
	void RemoveInputMappingContext(UInputMappingContext* ContextToRemove);

	// 클라이밍 전용 매핑 컨택스트 및 입력 인풋 선언
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* ClimbMappingContext;
	
	// 클라이밍 이동 액션
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ClimbMoveAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	// 클라이밍 인풋 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbAction;

	// 클라이밍 도약 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ClimbHopAction;

#pragma endregion

#pragma region InputCallback

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	// 유데미 Climb 핸들 함수들 
	//void HandleGroundMovementInput(const FInputActionValue& Value);
	void HandleClimbMovementInput(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// 클라이밍 시작 함수
	void OnClimbActionStarted(const FInputActionValue& Value);

	// 클라이밍 도약 액션이 시작될 때 호출될 콜백 함수 
	void OnClimbHopActionStarted(const FInputActionValue& Value);

#pragma endregion

	

public:

	// 슬라이딩시 이동가능 여부 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Movement")
	bool bCanMove = true;  // 이동 가능 여부

	// 빔이동시 빔위에 있는지 여부 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Movement")
	bool bIsOnBeam = false;  // 빔 위에 있는지 확인 변수

	// 은폐 커버 이동시 커버 상태에 있는지 확인하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Movement")
	bool bIsCover = false;


protected:

	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:

	

	// 모션 워핑 컴포넌트 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion Warping", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarpingComp;

	// 빔 이동 시 추적 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Movement")
	void MoveAlongBeam();

	// 은엄폐 인풋 이동 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player Movement")
	void InputMoving();

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// Blueprint와 다른 컴포넌트에서 접근할 수 있는 Getter
	//FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComp; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Motion Warping")
	UMotionWarpingComponent* GetMotionWarpingComp() const
	{
		return MotionWarpingComp;
	}


	// Getter 함수 
	FORCEINLINE UCustomMovementComponent* GetCustomMovementComponent() const { return CustomMovementComponent; }
};


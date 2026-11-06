#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "BallPawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

/**
 * Physics-driven ball the player controls, followed by a third-person camera.
 */
UCLASS()
class JUMPINGBALL_API ABallPawn : public APawn
{
	GENERATED_BODY()

public:
	ABallPawn();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Removes the mapping context added in SetupPlayerInputComponent. */
	virtual void UnPossessed() override;

	/** Handles MoveAction; Value carries the 2D input axis. */
	void Move(const FInputActionValue& Value);

	/** Handles LookPitchAction; Value carries the 1D pitch axis. */
	void LookPitch(const FInputActionValue& Value);

	/** Steers the spring arm: yaw follows the ball's motion, pitch follows CameraPitch. */
	void UpdateCamera(float DeltaTime);

	/** Visible sphere; root component and physics body. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BallMesh;

	/** Keeps the camera at a fixed offset behind the ball. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	/** Mapping context added to the local player's Enhanced Input subsystem. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** 2D axis action that rolls the ball. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Action that makes the ball jump. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	/** 1D axis action that tilts the camera up and down. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookPitchAction;

	/** Torque applied per unit of move input; tweak on a placed instance while playtesting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RollTorque = 300.0f;

	/** Ball speed (cm/s) above which the camera yaw starts following the direction of travel. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraFollowSpeedThreshold = 100.f;

	/** How quickly the camera yaw catches up with the direction of travel. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraYawInterpSpeed = 3.f;

	/** Lowest allowed camera pitch in degrees (negative looks down). */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float CameraPitchMin = -75.f;

	/** Highest allowed camera pitch in degrees. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float CameraPitchMax = -5.f;

	/** Degrees of pitch per unit of look input. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPitchSensitivity = 1.f;

	/** Latest 2D move input; transient runtime state. */
	FVector2D MoveInput = FVector2D::ZeroVector;

	/** Current camera pitch in degrees; seeded from the spring arm in BeginPlay. */
	float CameraPitch = -20.f;
};

#include "BallPawn.h"
#include "JumpingBall.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"

static TAutoConsoleVariable<bool> CVarBallDebug(
	TEXT("jb.Ball.Debug"), false,
	TEXT("Draw ball input/velocity debug lines and the on-screen input readout."),
	ECVF_Cheat);

ABallPawn::ABallPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	SetRootComponent(BallMesh);

	BallMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	BallMesh->SetSimulatePhysics(true);
	BallMesh->SetNotifyRigidBodyCollision(true);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(BallMesh);
	// The ball rolls, so the arm must ignore the root's rotation or the camera would spin with it.
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ABallPawn::BeginPlay()
{
	Super::BeginPlay();

	// Seed from the arm so a pitch set on the Blueprint and the runtime value can't disagree.
	CameraPitch = SpringArm->GetComponentRotation().Pitch;

	UE_LOG(LogJumpingBall, Log, TEXT("%s: BeginPlay"), *GetName());
}

void ABallPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Movement only with input, but the camera below must update every frame: the ball keeps rolling after release.
	if (!MoveInput.IsNearlyZero())
	{
		// Yaw only, so camera pitch never tilts the push into or away from the ground.
		const FRotationMatrix YawMatrix(FRotator(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f));
		const FVector Forward = YawMatrix.GetUnitAxis(EAxis::X);
		const FVector Right = YawMatrix.GetUnitAxis(EAxis::Y);
		// Clamp rather than normalize: keeps analog stick magnitude, stops diagonals on keys exceeding 1.
		const FVector Direction = (Forward * MoveInput.Y + Right * MoveInput.X).GetClampedToMaxSize(1.0f);

		// Rolling toward Direction means spinning about the horizontal axis perpendicular to it.
		const FVector Torque = FVector::CrossProduct(FVector::UpVector, Direction) * RollTorque;
		BallMesh->AddTorqueInRadians(Torque, NAME_None, true);

		// Shows intended direction: if the ball rolls against this line, the torque sign is wrong, not the direction.
		if (CVarBallDebug.GetValueOnGameThread())
		{
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Direction * 200.0f, FColor::Red, false, -1.0f, 0, 3.0f);
		}
	}

	UpdateCamera(DeltaTime);
}

void ABallPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// DefaultInput.ini sets EnhancedInputComponent as the default class, so a failed cast is a config bug.
	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABallPawn::Move);
		// Completed delivers a zero value on release, so Move can clear the stored input.
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ABallPawn::Move);
	}
	else
	{
		UE_LOG(LogJumpingBall, Error, TEXT("%s: MoveAction is not set; assign it in the Blueprint's Input defaults"), *GetName());
	}

	if (LookPitchAction)
	{
		// Triggered only: the value is a delta, so a zero on release would change nothing.
		EnhancedInput->BindAction(LookPitchAction, ETriggerEvent::Triggered, this, &ABallPawn::LookPitch);
	}
	else
	{
		UE_LOG(LogJumpingBall, Error, TEXT("%s: LookPitchAction is not set; assign it in the Blueprint's Input defaults"), *GetName());
	}

	// A missing asset on BP_Ball would otherwise leave a ball that silently ignores input.
	if (!DefaultMappingContext)
	{
		UE_LOG(LogJumpingBall, Error, TEXT("%s: DefaultMappingContext is not set; assign it in the Blueprint's Input defaults"), *GetName());
		return;
	}

	// Only called when a PlayerController possesses this pawn, so the controller and its local player exist here.
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogJumpingBall, Error, TEXT("%s: SetupPlayerInputComponent called without a PlayerController"), *GetName());
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (!Subsystem)
	{
		UE_LOG(LogJumpingBall, Error, TEXT("%s: no EnhancedInputLocalPlayerSubsystem for %s"), *GetName(), *PlayerController->GetName());
		return;
	}

	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

void ABallPawn::UnPossessed()
{
	// Super clears Controller, so grab it first.
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());

	// Not an error: AI controllers never added a context, and during teardown the local player may already be gone.
	if (DefaultMappingContext && PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(DefaultMappingContext);
		}
	}

	Super::UnPossessed();
}

void ABallPawn::Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();

	// Fires every frame while held, so show it on screen (fixed key overwrites one line) instead of flooding the log.
	if (CVarBallDebug.GetValueOnGameThread() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Green, FString::Printf(TEXT("Move: %s"), *MoveInput.ToString()));
	}
}

void ABallPawn::LookPitch(const FInputActionValue& Value)
{
	CameraPitch = FMath::Clamp(CameraPitch + Value.Get<float>() * CameraPitchSensitivity, CameraPitchMin, CameraPitchMax);
}

void ABallPawn::UpdateCamera(float DeltaTime)
{
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;

	// Green = where the ball is actually going (red in Tick = where input asks it to go); the camera settles toward green.
	if (CVarBallDebug.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + HorizontalVelocity.GetSafeNormal() * 200.0f, FColor::Green, false, -1.0f, 0, 3.0f);
	}

	const FRotator Current = SpringArm->GetComponentRotation();
	float NewYaw = Current.Yaw;

	// Below the threshold the direction of travel is noise (settling, jitter), so hold the current yaw.
	if (HorizontalVelocity.SizeSquared() > FMath::Square(CameraFollowSpeedThreshold))
	{
		const float TargetYaw = HorizontalVelocity.Rotation().Yaw;
		// RInterpTo takes the shortest way around, so crossing ±180° doesn't spin the camera the long way.
		NewYaw = FMath::RInterpTo(FRotator(0.0f, Current.Yaw, 0.0f), FRotator(0.0f, TargetYaw, 0.0f), DeltaTime, CameraYawInterpSpeed).Yaw;
	}

	// Arm uses absolute rotation, so this is independent of the ball's spin.
	SpringArm->SetWorldRotation(FRotator(CameraPitch, NewYaw, 0.0f));
}

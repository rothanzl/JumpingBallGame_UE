#include "HelloActor.h"
#include "JumpingBall.h"

AHelloActor::AHelloActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHelloActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogJumpingBall, Log, TEXT("%s: BeginPlay (SpinSpeed = %.1f deg/s)"), *GetName(), SpinSpeed);
}

void AHelloActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Scale by DeltaTime so the spin rate is frame-rate independent.
	AddActorLocalRotation(FRotator(0.0f, SpinSpeed * DeltaTime, 0.0f));
}

void AHelloActor::SayHello()
{
	UE_LOG(LogJumpingBall, Log, TEXT("Hello from %s!"), *GetName());
}

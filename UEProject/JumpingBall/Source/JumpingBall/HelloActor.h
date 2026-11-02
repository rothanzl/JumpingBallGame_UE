#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HelloActor.generated.h"

/**
 * Simple actor that spins around its vertical axis and can say hello.
 */
UCLASS()
class JUMPINGBALL_API AHelloActor : public AActor
{
	GENERATED_BODY()

public:
	AHelloActor();

	/** Rotation speed around the Z axis, in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hello")
	float SpinSpeed = 90.0f;

	/** Logs a greeting to LogJumpingBall. */
	UFUNCTION(BlueprintCallable, Category = "Hello")
	void SayHello();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
};

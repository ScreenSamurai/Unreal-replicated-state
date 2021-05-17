// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicatior.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Tranform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivetive, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float LarpRatio) const 
	{
		return FMath::CubicInterp(StartLocation, StartDerivetive, TargetLocation, TargetDerivative, LarpRatio);
	}
	FVector InterpolateDerivative(float LarpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivetive, TargetLocation, TargetDerivative, LarpRatio);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CARMOVEMENSYSTEM_API UGoKartMovementReplicatior : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicatior();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	void UpdateServerState(const FGoKartMove& Move);
	void ClienTick(float DeltaTime);

	void InterpolateLocation(const FHermiteCubicSpline& Spline, float LarpRation);
	void InterpolateVelocity(const FHermiteCubicSpline& Spline, float LarpRation);
	void InterpolateRoration(float LarpRation);
	FHermiteCubicSpline CreateSpline();
	float VelocityToDerivative();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UFUNCTION()
	void OnRep_ServerState();
	void AutonomousProxy_OnRep_ServerState();
	void SimulateProxy_OnRep_ServerState();


	TArray<FGoKartMove> UnacknowLedgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdate;
	FTransform ClientStartTrasform;
	FVector ClientStartVelocity;

	float ClientSimulatedtime;

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;

	UPROPERTY()
	USceneComponent* MashOffsetRoot;
	UFUNCTION(BlueprintCallable)
	void SetMashOffsetRoot(USceneComponent* Root) { MashOffsetRoot = Root; }
};

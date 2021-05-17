// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicatior.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicatior::UGoKartMovementReplicatior()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}


// Called when the game starts
void UGoKartMovementReplicatior::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}


// Called every frame
void UGoKartMovementReplicatior::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementComponent == nullptr)return;
	FGoKartMove LastMove= MovementComponent->GetLastMove();
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowLedgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}
	//We are the server and in control of the pawn
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
	}
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClienTick(DeltaTime);
	}
}

void UGoKartMovementReplicatior::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMovementReplicatior, ServerState);
}

void UGoKartMovementReplicatior::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMove;

	for (const FGoKartMove& Move : UnacknowLedgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMove.Add(Move);
		}
	}
	UnacknowLedgedMoves = NewMove;
}

void UGoKartMovementReplicatior::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Tranform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicatior::ClienTick(float DeltaTime)
{

	ClientTimeSinceUpdate += DeltaTime;
	if (ClientTimeBetweenLastUpdate < KINDA_SMALL_NUMBER) return;
	if (MovementComponent == nullptr)return;

	FHermiteCubicSpline Spline = CreateSpline();
	float LarpRation = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdate;

	InterpolateLocation(Spline, LarpRation);

	InterpolateVelocity(Spline, LarpRation);

	InterpolateRoration(LarpRation);
}

void UGoKartMovementReplicatior::InterpolateLocation(const FHermiteCubicSpline& Spline, float LarpRation)
{
	FVector NewLocation = Spline.InterpolateLocation(LarpRation);
	if (MashOffsetRoot != nullptr)
	{
		MashOffsetRoot->SetWorldLocation(NewLocation);
	}
}

void UGoKartMovementReplicatior::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LarpRation)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LarpRation);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}


void UGoKartMovementReplicatior::InterpolateRoration(float LarpRation)
{
	FQuat TargetRotation = ServerState.Tranform.GetRotation();
	FQuat StartRotation = ClientStartTrasform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LarpRation);

	if (MashOffsetRoot != nullptr)
	{
		MashOffsetRoot->SetWorldRotation(NewRotation);
	}
}

FHermiteCubicSpline UGoKartMovementReplicatior::CreateSpline()
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Tranform.GetLocation();
	Spline.StartLocation = ClientStartTrasform.GetLocation();
	Spline.StartDerivetive = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();
	return Spline;
}

float UGoKartMovementReplicatior::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdate * 100;
}

void UGoKartMovementReplicatior::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulateProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMovementReplicatior::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr)return;

	GetOwner()->SetActorTransform(ServerState.Tranform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowLedgedMoves)
	{
		MovementComponent->SimuletMove(Move);
	}
}

void UGoKartMovementReplicatior::SimulateProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr)return;

	ClientTimeBetweenLastUpdate = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MashOffsetRoot !=nullptr)
	{
		ClientStartTrasform.SetLocation(MashOffsetRoot->GetComponentLocation());
		ClientStartTrasform.SetRotation(MashOffsetRoot->GetComponentQuat());
	}
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Tranform);
}

void UGoKartMovementReplicatior::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr)return;

	ClientSimulatedtime += Move.DeltaTime;
	MovementComponent->SimuletMove(Move);

	UpdateServerState(Move);
}

bool UGoKartMovementReplicatior::Server_SendMove_Validate(FGoKartMove Move)
{
	float ProposedTime = ClientSimulatedtime + Move.DeltaTime;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->GetTimeSeconds();
	if (!ClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Warning, TEXT("Client is running too fast!"));
	}
	if (!Move.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Received invalid move!"));
		return false;
	}
	return true;
}
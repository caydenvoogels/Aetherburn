#include "ThunderlordContentLibrary.h"
#include "ThunderlordAnimInstance.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"

#include "UObject/UnrealType.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimationStateGraph.h"
#include "AnimationTransitionGraph.h"
#include "AnimStateNode.h"
#include "AnimStateEntryNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_TransitionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraphPin.h"
#endif

#if WITH_EDITOR
namespace
{
	UEdGraphPin* FindPinByName(UEdGraphNode* Node, FName Name, EEdGraphPinDirection Direction)
	{
		if (!Node) return nullptr;
		for (UEdGraphPin* Pin : Node->Pins)
			if (Pin && Pin->PinName == Name && Pin->Direction == Direction) return Pin;
		return nullptr;
	}

	UEdGraphPin* FindFirstPosePin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
	{
		if (!Node) return nullptr;
		for (UEdGraphPin* Pin : Node->Pins)
			if (Pin && Pin->Direction == Direction && Pin->PinType.PinCategory == UAnimationGraphSchema::PC_Struct) return Pin;
		return nullptr;
	}

	template<typename NodeType>
	NodeType* AddGraphNode(UEdGraph* Graph, FVector2D Position)
	{
		if (!Graph) return nullptr;
		NodeType* Node = NewObject<NodeType>(Graph);
		Graph->AddNode(Node, true, false);
		Node->CreateNewGuid();
		Node->NodePosX = Position.X;
		Node->NodePosY = Position.Y;
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();
		return Node;
	}

	bool ConnectPose(UEdGraph* Graph, UEdGraphPin* Output, UEdGraphPin* Input)
	{
		return Graph && Output && Input && Graph->GetSchema()->TryCreateConnection(Output, Input);
	}

	bool AddBoolTransitionRule(UAnimStateTransitionNode* Transition, UFunction* Predicate)
	{
		UEdGraph* Graph = Transition ? Transition->BoundGraph : nullptr;
		if (!Graph || !Predicate) return false;
		UAnimGraphNode_TransitionResult* Result = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
			if ((Result = Cast<UAnimGraphNode_TransitionResult>(Node))) break;
		if (!Result) return false;
		UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(Graph);
		Graph->AddNode(Call, true, false);
		Call->CreateNewGuid();
		Call->NodePosX = -220;
		Call->NodePosY = -40;
		Call->SetFromFunction(Predicate);
		Call->AllocateDefaultPins();
		UEdGraphPin* ReturnValue = FindPinByName(Call, UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output);
		UEdGraphPin* Condition = FindPinByName(Result, TEXT("bCanEnterTransition"), EGPD_Input);
		return ReturnValue && Condition && Graph->GetSchema()->TryCreateConnection(ReturnValue, Condition);
	}

	UAnimStateNode* AddNamedState(UAnimationStateMachineGraph* Graph, const FString& Name, FVector2f Position)
	{
		UAnimStateNode* Template = NewObject<UAnimStateNode>(GetTransientPackage());
		UAnimStateNode* State = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate(Graph, Template, Position, false);
		if (!State) return nullptr;
		State->OnRenameNode(Name);
		State->NodeComment = Name;
		if (State->GetStateName() != Name) return nullptr;
		State->BoundGraph->Modify();
		return State;
	}

	bool ConfigureStatePose(UAnimStateNode* State, UBlendSpace* Blend, UAnimSequence* Sequence, UAnimBlueprint* Blueprint)
	{
		if (!State || !State->BoundGraph) return false;
		UEdGraph* Graph = State->BoundGraph;
		UAnimGraphNode_StateResult* Result = State->GetResultNodeInsideState();
		if (!Result) return false;
		if (Blend)
		{
			UAnimGraphNode_BlendSpacePlayer* Player = AddGraphNode<UAnimGraphNode_BlendSpacePlayer>(Graph, FVector2D(-250, 0));
			if (!Player) return false;
			Player->SetAnimationAsset(Blend);
			UK2Node_VariableGet* Direction = NewObject<UK2Node_VariableGet>(Graph);
			Graph->AddNode(Direction, true, false);
			Direction->CreateNewGuid(); Direction->NodePosX = -560; Direction->NodePosY = -110;
			Direction->VariableReference.SetSelfMember(TEXT("Direction")); Direction->AllocateDefaultPins();
			UK2Node_VariableGet* Speed = NewObject<UK2Node_VariableGet>(Graph);
			Graph->AddNode(Speed, true, false);
			Speed->CreateNewGuid(); Speed->NodePosX = -560; Speed->NodePosY = 100;
			Speed->VariableReference.SetSelfMember(TEXT("Speed")); Speed->AllocateDefaultPins();
			const bool bDirection = Graph->GetSchema()->TryCreateConnection(FindPinByName(Direction, TEXT("Direction"), EGPD_Output), FindPinByName(Player, TEXT("X"), EGPD_Input));
			const bool bSpeed = Graph->GetSchema()->TryCreateConnection(FindPinByName(Speed, TEXT("Speed"), EGPD_Output), FindPinByName(Player, TEXT("Y"), EGPD_Input));
			return bDirection && bSpeed && ConnectPose(Graph, FindFirstPosePin(Player, EGPD_Output), State->GetPoseSinkPinInsideState());
		}
		if (!Sequence) return false;
	UAnimGraphNode_SequencePlayer* Player = AddGraphNode<UAnimGraphNode_SequencePlayer>(Graph, FVector2D(-250, 0));
	if (!Player) return false;
	Player->SetAnimationAsset(Sequence);
	Player->Node.SetLoopAnimation(State->GetStateName() == TEXT("Fall"));
	return ConnectPose(Graph, FindFirstPosePin(Player, EGPD_Output), State->GetPoseSinkPinInsideState());
}
}
#endif

bool UThunderlordContentLibrary::ConfigureBlendSpace(UBlendSpace* Space, const TArray<UAnimSequence*>& Clips, const TArray<FVector>& Coordinates, float MaximumSpeed)
{
#if WITH_EDITOR
	if (!Space || Clips.Num() != Coordinates.Num() || Clips.IsEmpty()) return false;
	for (UAnimSequence* Clip : Clips) if (!Clip || Clip->GetSkeleton() != Space->GetSkeleton()) return false;
	Space->Modify();
	FStructProperty* Axes = FindFProperty<FStructProperty>(UBlendSpace::StaticClass(), TEXT("BlendParameters"));
	if (!Axes) return false;
	FBlendParameter* Direction = Axes->ContainerPtrToValuePtr<FBlendParameter>(Space, 0);
	Direction->DisplayName = TEXT("Direction");
	Direction->Min = -180;
	Direction->Max = 180;
	Direction->GridNum = 8;
	Direction->bWrapInput = true;
	FBlendParameter* Speed = Axes->ContainerPtrToValuePtr<FBlendParameter>(Space, 1);
	Speed->DisplayName = TEXT("Speed");
	Speed->Min = 0;
	Speed->Max = MaximumSpeed;
	Speed->GridNum = 4;
	while (Space->GetNumberOfBlendSamples() > 0) Space->DeleteSample(Space->GetNumberOfBlendSamples() - 1);
	for (int32 Index = 0; Index < Clips.Num(); ++Index)
	{
		if (Space->AddSample(Clips[Index], Coordinates[Index]) == INDEX_NONE) return false;
	}
	Space->ValidateSampleData();
	Space->ResampleData();
	Space->PostEditChange();
	Space->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UThunderlordContentLibrary::ApplyCrouchPosture(UAnimSequence* Output, UAnimSequence* Walk, UAnimSequence* Crouch)
{
#if WITH_EDITOR
	if (!Output || !Walk || !Crouch || Output->GetSkeleton() != Walk->GetSkeleton() || Output->GetSkeleton() != Crouch->GetSkeleton()) return false;
	const IAnimationDataModel* Model = Output->GetDataModel();
	const IAnimationDataModel* WalkModel = Walk->GetDataModel();
	const IAnimationDataModel* CrouchModel = Crouch->GetDataModel();
	TArray<FName> Names;
	Model->GetBoneTrackNames(Names);
	IAnimationDataController& Controller = Output->GetController();
	Controller.OpenBracket(FText::FromString(TEXT("Adapt authored crouch posture to directional gait")), false);
	const int32 Frames = Model->GetNumberOfFrames();
	for (FName Bone : Names)
	{
		if (!WalkModel->IsValidBoneTrackName(Bone) || !CrouchModel->IsValidBoneTrackName(Bone)) continue;
		TArray<FTransform> Directional;
		Model->GetBoneTrackTransforms(Bone, Directional);
		TArray<FVector> Positions, Scales;
		TArray<FQuat> Rotations;
		for (int32 Key = 0; Key <= Frames; ++Key)
		{
			const double Phase = static_cast<double>(Key) / FMath::Max(1, Frames);
			const FTransform W = WalkModel->EvaluateBoneTrackTransform(Bone, FFrameTime::FromDecimal(Phase * WalkModel->GetNumberOfFrames()), EAnimInterpolationType::Linear);
			const FTransform C = CrouchModel->EvaluateBoneTrackTransform(Bone, FFrameTime::FromDecimal(Phase * CrouchModel->GetNumberOfFrames()), EAnimInterpolationType::Linear);
			const FTransform& D = Directional[FMath::Min(Key, Directional.Num() - 1)];
			// Transfer the authored crouch offset in local bone space while
			// preserving the directional walking cycle. This creates real keyed
			// knees, hips, spine and arms, rather than sinking the whole mesh.
			Rotations.Add((C.GetRotation() * W.GetRotation().Inverse() * D.GetRotation()).GetNormalized());
			Positions.Add(D.GetTranslation() + C.GetTranslation() - W.GetTranslation());
			Scales.Add(D.GetScale3D());
		}
		Controller.SetBoneTrackKeys(Bone, Positions, Rotations, Scales, false);
	}
	Controller.CloseBracket(false);
	Output->PostEditChange();
	Output->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UThunderlordContentLibrary::MakeAnimationInPlace(UAnimSequence* Animation, bool bLockVertical)
{
#if WITH_EDITOR
	if (!Animation || !Animation->GetSkeleton() || !Animation->GetDataModel()) return false;
	const FReferenceSkeleton& ReferenceSkeleton = Animation->GetSkeleton()->GetReferenceSkeleton();
	if (ReferenceSkeleton.GetNum() == 0) return false;
	const FName RootBone = ReferenceSkeleton.GetBoneName(0);
	const IAnimationDataModel* Model = Animation->GetDataModel();
	if (!Model->IsValidBoneTrackName(RootBone)) return false;
	const int32 Frames = Model->GetNumberOfFrames();
	if (Frames < 1) return false;
	const FTransform FirstFrame = Model->EvaluateBoneTrackTransform(
		RootBone, FFrameTime(0), EAnimInterpolationType::Linear);
	TArray<FVector> Positions;
	TArray<FQuat> Rotations;
	TArray<FVector> Scales;
	Positions.Reserve(Frames + 1);
	Rotations.Reserve(Frames + 1);
	Scales.Reserve(Frames + 1);
	for (int32 Frame = 0; Frame <= Frames; ++Frame)
	{
		const FTransform Transform = Model->EvaluateBoneTrackTransform(
			RootBone, FFrameTime(Frame), EAnimInterpolationType::Linear);
		FVector Position = Transform.GetTranslation();
		Position.X = FirstFrame.GetTranslation().X;
		Position.Y = FirstFrame.GetTranslation().Y;
		if (bLockVertical) Position.Z = FirstFrame.GetTranslation().Z;
		Positions.Add(Position);
		Rotations.Add(Transform.GetRotation());
		Scales.Add(Transform.GetScale3D());
	}
	IAnimationDataController& Controller = Animation->GetController();
	Controller.OpenBracket(FText::FromString(TEXT("Remove root translation for in-place slide")), false);
	const bool bTrackSet = Controller.SetBoneTrackKeys(RootBone, Positions, Rotations, Scales, false);
	Controller.CloseBracket(false);
	if (!bTrackSet) return false;
	Animation->PostEditChange();
	Animation->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UThunderlordContentLibrary::IsAnimationInPlace(UAnimSequence* Animation, bool bLockVertical, float Tolerance)
{
	if (!Animation || !Animation->GetSkeleton() || !Animation->GetDataModel()) return false;
	const FReferenceSkeleton& ReferenceSkeleton = Animation->GetSkeleton()->GetReferenceSkeleton();
	if (ReferenceSkeleton.GetNum() == 0) return false;
	const FName RootBone = ReferenceSkeleton.GetBoneName(0);
	const IAnimationDataModel* Model = Animation->GetDataModel();
	if (!Model->IsValidBoneTrackName(RootBone) || Model->GetNumberOfFrames() < 1) return false;
	const FVector First = Model->EvaluateBoneTrackTransform(RootBone, FFrameTime(0), EAnimInterpolationType::Linear).GetTranslation();
	for (int32 Frame = 1; Frame <= Model->GetNumberOfFrames(); ++Frame)
	{
		const FVector Position = Model->EvaluateBoneTrackTransform(RootBone, FFrameTime(Frame), EAnimInterpolationType::Linear).GetTranslation();
		if (FVector2D(Position.X - First.X, Position.Y - First.Y).Size() > Tolerance) return false;
		if (bLockVertical && FMath::Abs(Position.Z - First.Z) > Tolerance) return false;
	}
	return true;
}

bool UThunderlordContentLibrary::ConfigureAnimationBlueprint(UAnimBlueprint* Blueprint, UBlendSpace* Standing, UBlendSpace* Crouch, UAnimSequence* Slide, UAnimSequence* Jump, UAnimSequence* Fall)
{
#if WITH_EDITOR
	if (!Blueprint || !Standing || !Crouch || !Slide || !Jump || !Fall) return false;
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	UAnimationGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AllGraphs) if ((AnimGraph = Cast<UAnimationGraph>(Graph))) break;
	if (!AnimGraph) return false;
	AnimGraph->Modify();
	UAnimGraphNode_Root* Root = nullptr;
	const TArray<UEdGraphNode*> ExistingNodes = AnimGraph->Nodes;
	for (UEdGraphNode* Node : ExistingNodes)
	{
		if (UAnimGraphNode_Root* Found = Cast<UAnimGraphNode_Root>(Node)) Root = Found;
		else
		{
			// Remove the old state's bound rule and pose graphs as well as its node.
			// Otherwise Unreal still compiles orphaned rules that refer to removed predicates.
			if (UAnimGraphNode_StateMachine* OldMachine = Cast<UAnimGraphNode_StateMachine>(Node))
			{
				if (UAnimationStateMachineGraph* OldStateGraph = OldMachine->EditorStateMachineGraph)
				{
					const TArray<UEdGraphNode*> OldStateNodes = OldStateGraph->Nodes;
					for (UEdGraphNode* OldStateNode : OldStateNodes)
					{
						if (UAnimStateNode* OldState = Cast<UAnimStateNode>(OldStateNode))
						{
							TArray<UAnimStateTransitionNode*> OldTransitions;
							OldState->GetTransitionList(OldTransitions);
							for (UAnimStateTransitionNode* OldTransition : OldTransitions)
								if (OldTransition && OldTransition->BoundGraph)
									FBlueprintEditorUtils::RemoveGraph(Blueprint, OldTransition->BoundGraph, EGraphRemoveFlags::None);

							if (OldState->BoundGraph)
								FBlueprintEditorUtils::RemoveGraph(Blueprint, OldState->BoundGraph, EGraphRemoveFlags::None);
						}
					}
				}
			}
			Node->DestroyNode();
		}
	}
	if (!Root) return false;
	UAnimGraphNode_StateMachine* Machine = AddGraphNode<UAnimGraphNode_StateMachine>(AnimGraph, FVector2D(-200, 20));
	if (!Machine || !Machine->EditorStateMachineGraph) return false;
	UAnimationStateMachineGraph* StateGraph = Machine->EditorStateMachineGraph;
	StateGraph->Modify();
	TArray<UAnimStateNode*> States;
	const TArray<FString> Names = { TEXT("Locomotion"), TEXT("Crouch"), TEXT("Slide"), TEXT("Jump"), TEXT("Fall") };
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		UAnimStateNode* State = AddNamedState(StateGraph, Names[Index], FVector2f((Index % 3) * 280.0f, (Index / 3) * 180.0f));
		if (!State) return false;
		States.Add(State);
	}
	if (!StateGraph->EntryNode || !StateGraph->GetSchema()->TryCreateConnection(StateGraph->EntryNode->GetOutputPin(), States[0]->GetInputPin())) return false;
	if (!ConfigureStatePose(States[0], Standing, nullptr, Blueprint) || !ConfigureStatePose(States[1], Crouch, nullptr, Blueprint) ||
		!ConfigureStatePose(States[2], nullptr, Slide, Blueprint) || !ConfigureStatePose(States[3], nullptr, Jump, Blueprint) ||
		!ConfigureStatePose(States[4], nullptr, Fall, Blueprint)) return false;
	const FName Predicates[] = { TEXT("IsLocomotionState"), TEXT("IsCrouchState"), TEXT("IsSlideState"), TEXT("IsJumpState"), TEXT("IsFallState") };
	UClass* AnimClass = UThunderlordAnimInstance::StaticClass();
	for (int32 Source = 0; Source < States.Num(); ++Source)
	{
		for (int32 Target = 0; Target < States.Num(); ++Target)
		{
			if (Source == Target) continue;
			if (!StateGraph->GetSchema()->TryCreateConnection(States[Source]->GetOutputPin(), States[Target]->GetInputPin())) return false;
			TArray<UAnimStateTransitionNode*> Transitions;
			States[Source]->GetTransitionList(Transitions);
			UAnimStateTransitionNode* Transition = nullptr;
			for (UAnimStateTransitionNode* Candidate : Transitions)
				if (Candidate && Candidate->GetNextState() == States[Target]) { Transition = Candidate; break; }
			if (!Transition) return false;
			// Enter Jump immediately on input; blend into Fall so its open-arm pose does not pop.
			Transition->CrossfadeDuration = Target == 3 ? 0.0f : (Target == 4 ? 0.24f : 0.18f);
			Transition->LogicType = ETransitionLogicType::TLT_StandardBlend;
			Transition->bAutomaticRuleBasedOnSequencePlayerInState = false;
			UFunction* Predicate = AnimClass->FindFunctionByName(Predicates[Target]);
			if (!AddBoolTransitionRule(Transition, Predicate)) return false;
		}
	}
	UEdGraphPin* MachineOutput = FindFirstPosePin(Machine, EGPD_Output);
	UEdGraphPin* RootInput = FindFirstPosePin(Root, EGPD_Input);
	if (!ConnectPose(AnimGraph, MachineOutput, RootInput)) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	return Blueprint->Status != BS_Error;
#else
	return false;
#endif
}

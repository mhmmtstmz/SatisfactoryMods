

#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableFactory.h"
#include "Buildables/FGBuildableConveyorAttachment.h"
#include "FGFactoryConnectionComponent.h"
#include "FGInventoryComponent.h"
#include "Resources/FGNoneDescriptor.h"
#include "LBBuild_ModularLoadBalancer.generated.h"

UENUM(BlueprintType)
enum class ELoaderType : uint8
{
	Normal = 0 UMETA(DisplayName = "Normal"),
	Overflow = 1 UMETA(DisplayName = "Overflow"),
	Filter = 2 UMETA(DisplayName = "Filter")
};

class ALBBuild_ModularLoadBalancer;
USTRUCT()
struct LOADBALANCERS_API FLBBalancerData_Filters
{
	GENERATED_BODY()

	FLBBalancerData_Filters() = default;
	FLBBalancerData_Filters(ALBBuild_ModularLoadBalancer* Balancer)
	{
		mBalancer.Add(Balancer);
		mOutputIndex = 0;
	}
	
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALBBuild_ModularLoadBalancer>> mBalancer;
	
	UPROPERTY(SaveGame)
	int mOutputIndex = 0;

	void GetBalancers(TArray<ALBBuild_ModularLoadBalancer*> Out)
	{
		for (TWeakObjectPtr<ALBBuild_ModularLoadBalancer> Balancer : mBalancer)
			if(Balancer.IsValid())
				Out.AddUnique(Balancer.Get());
	}
};

USTRUCT()
struct LOADBALANCERS_API FLBBalancerData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALBBuild_ModularLoadBalancer>> mConnectedOutputs;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALBBuild_ModularLoadBalancer>> mConnectedInputs;

	// for MAYBE LATER IS NOT USED!
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ALBBuild_ModularLoadBalancer>> mOverflowBalancer;

	UPROPERTY(SaveGame)
	int mInputIndex = 0;

	UPROPERTY(SaveGame)
	int mOverflowIndex = 0;

	UPROPERTY(SaveGame)
	TMap<TSubclassOf<UFGItemDescriptor>, int> mOutputIndex;
	
	UPROPERTY(SaveGame)
	TMap<TSubclassOf<UFGItemDescriptor>, FLBBalancerData_Filters> mFilterMap;

	void GetInputBalancers(TArray<ALBBuild_ModularLoadBalancer*>& Out);

	bool HasAnyValidFilter() const;

	bool HasAnyValidOverflow() const;

	int GetOutputIndexFromItem(TSubclassOf<UFGItemDescriptor> Item, bool IsFilter = false);

	void SetFilterItemForBalancer(ALBBuild_ModularLoadBalancer* Balancer, TSubclassOf<UFGItemDescriptor> Item, TSubclassOf<UFGItemDescriptor> OldItem);

	void RemoveBalancer(ALBBuild_ModularLoadBalancer* Balancer, TSubclassOf<UFGItemDescriptor> OldItem);

	bool HasItemFilterBalancer(TSubclassOf<UFGItemDescriptor> Item) const;

	TArray<ALBBuild_ModularLoadBalancer*> GetBalancerForFilters(TSubclassOf<UFGItemDescriptor> Item);
};

/**
 *
 */
UCLASS()
class LOADBALANCERS_API ALBBuild_ModularLoadBalancer : public AFGBuildableConveyorAttachment
{
	GENERATED_BODY()
public:

	ALBBuild_ModularLoadBalancer();

	// Begin AActor interface
	virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor interface

	UFUNCTION()
	void OnOutputItemRemoved( TSubclassOf<UFGItemDescriptor> itemClass, int32 numRemoved );

	/** Apply this module to the group (Called on BeginPlay should not need to call again) */
	void ApplyGroupModule();

	/** Is this module the Leader return true if yes */
	FORCEINLINE bool IsLeader() const { return GroupLeader == this; }

	UFUNCTION(BlueprintCallable, Category="Modular Loader")
	void SetFilteredItem(TSubclassOf<UFGItemDescriptor> ItemClass);

	/** Make this Loader to new GroupLeader (Tell all group modules the new leader and force an update) */
	void ApplyLeader();

	/* Get ALL valid GroupModules */
	TArray<ALBBuild_ModularLoadBalancer*> GetGroupModules() const;

	/* Overflow get pointer for the loader */
	bool SendToOverflowBalancer(FInventoryItem Item);

	/* Overflow get pointer for the loader */
	bool SendToFilterBalancer(FInventoryItem Item);

	/* Overflow get pointer for the loader */
	bool SendToNormalBalancer(FInventoryItem Item);

	/* Do we have a overflow loader? (return true if pointer valid) */
	FORCEINLINE bool HasOverflowModule() const
	{
		if(GroupLeader)
			return GroupLeader->mNormalLoaderData.HasAnyValidOverflow();
		return false;
	}

	FORCEINLINE bool HasFilterModule() const
	{
		if(GroupLeader)
			return GroupLeader->mNormalLoaderData.HasAnyValidFilter();
		return false;
	}

	/* Some native helper */
	FORCEINLINE bool IsOverflowModule() const { return mLoaderType == ELoaderType::Overflow; }
	FORCEINLINE bool IsFilterModule() const { return mLoaderType == ELoaderType::Filter; }
	FORCEINLINE bool IsNormalModule() const { return mLoaderType == ELoaderType::Normal; }

	UPROPERTY(BlueprintReadWrite, SaveGame, Replicated)
	ALBBuild_ModularLoadBalancer* GroupLeader;

	/** Current Filtered Item (if Filter) */
	UPROPERTY(BlueprintReadOnly, Replicated, SaveGame)
	TSubclassOf<UFGItemDescriptor> mFilteredItem = UFGNoneDescriptor::StaticClass();

	// Dont need to be Saved > CAN SET BY CPP
	UPROPERTY(Transient)
	UFGFactoryConnectionComponent* MyOutputConnection;

	// Dont need to be Saved > CAN SET BY CPP
	UPROPERTY(Transient)
	UFGFactoryConnectionComponent* MyInputConnection;

	/** What type is this loader? */
	UPROPERTY(EditDefaultsOnly, Category="ModularLoader")
	ELoaderType mLoaderType = ELoaderType::Normal;

	// Execute Logic by Leader > Factory_CollectInput_Implementation
	void Balancer_CollectInput();

private:
	/** Update our cached In and Outputs */
	void UpdateCache();

	/** Collect Logic for an Input
	 * Return true if the Item was store into a balancer
	 */
	bool CollectInput(ALBBuild_ModularLoadBalancer* Module);

	/* All Loaders */
	UPROPERTY(SaveGame)
	FLBBalancerData mNormalLoaderData;

	/** All our group modules */
	UPROPERTY(Replicated)
	TArray<TWeakObjectPtr<ALBBuild_ModularLoadBalancer>> mGroupModules;

	UPROPERTY(SaveGame)
	float mTimer;
protected:
	// Begin AActor Interface
	virtual void PostInitializeComponents() override;
	// End AActor Interface

	// Begin AFGBuildableFactory interface
	virtual void Factory_Tick(float dt) override;
	virtual void Factory_CollectInput_Implementation() override;
	// End
};
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ActionRPGGame.h"
#include "Attributes/GSAttributeComponent.h"
#include "GAAttributesStats.h"
#include "ARCharacterAttributes.h"

DEFINE_STAT(STAT_PostModifyAttribute);
DEFINE_STAT(STAT_PostEffectApplied);
DEFINE_STAT(STAT_OutgoingAttribute);
DEFINE_STAT(STAT_IncomingAttribute);
UARCharacterAttributes::UARCharacterAttributes(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Damage = 0;
	FireDamage = 0;
}
void UARCharacterAttributes::InitializeAttributes()
{

	
	PostModifyAttributeFunctions.Empty();
	IncomingModifyAttributeFunctions.Empty();
	OutgoingModifyAttributeFunctions.Empty();
	for (TFieldIterator<UFunction> FuncIt(GetClass(), EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
	{
		int32 FoundIndex = -1;
		FoundIndex = FuncIt->GetName().Find("PostAttribute");
		if (FoundIndex > -1)
		{
			FString postName = FuncIt->GetName();
			postName = postName.RightChop(14);
			FName keyIn = *postName;
			PostModifyAttributeFunctions.Add(keyIn, *FuncIt);
		}
	}
	//for (TFieldIterator<UFunction> FuncIt(GetClass(), EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
	//{
	//	if (FuncIt->GetMetaData("Category") == "Outgoing")
	//	{
	//		FString name = FuncIt->GetName();
	//		name = name.RightChop(9);
	//		//attempt to add function like Condition.Hex, which might happen to exist in blueprint.
	//		FName keyIn = *name;
	//		OutgoingModifyAttributeFunctions.Add(keyIn, *FuncIt);
	//		//then we will look for functions in C++.
	//		name = name.Replace(TEXT("_"), TEXT("."));
	//		keyIn = *name;
	//		OutgoingModifyAttributeFunctions.Add(keyIn, *FuncIt);
	//	}
	//}
	//for (TFieldIterator<UFunction> FuncIt(GetClass(), EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
	//{
	//	if (FuncIt->GetMetaData("Category") == "Incoming")
	//	{
	//		FString name = FuncIt->GetName();
	//		name = name.RightChop(9);
	//		name = name.Replace(TEXT("_"), TEXT("."));
	//		FName keyIn = *name;
	//		IncomingModifyAttributeFunctions.Add(keyIn, *FuncIt);
	//	}
	//}

	//for (TFieldIterator<UStructProperty> StrIt(GetClass(), EFieldIteratorFlags::IncludeSuper); StrIt; ++StrIt)
	//{
	//	FGAAttributeBase* attr = StrIt->ContainerPtrToValuePtr<FGAAttributeBase>(this);
	//	if (attr)
	//	{
	//		attr->InitializeAttribute();
	//	}
	//}
}

void UARCharacterAttributes::PostEffectApplied()
{
	SCOPE_CYCLE_COUNTER(STAT_PostEffectApplied);
	//FName Tag = SpecIn.MyTag.GetTagName();
	//FString nam = "OnEffectApplied_";
	//Tag = *Tag.ToString().Replace(TEXT("."), TEXT("_"));
	//nam = nam.Append(Tag.ToString());
	//UFunction* ModifyAttrFunc = GetClass()->FindFunctionByName(*nam);
}
void UARCharacterAttributes::PostEffectRemoved(const FGAEffectHandle& HandleIn, const FGAEffectSpec& SpecIn)
{
	for (const FGAAttributeModifier& mod : SpecIn.AttributeModifiers)
	{
		FName Tag = mod.Attribute.AttributeName;
		FString nam = "OnEffectApplied_";
		Tag = *Tag.ToString().Replace(TEXT("."), TEXT("_"));
		nam = nam.Append(Tag.ToString());
		UFunction* ModifyAttrFunc = GetClass()->FindFunctionByName(*nam);
	}
}

float UARCharacterAttributes::PreModifyAttribute(FGAAttributeData& AttributeMod, EGAModifierDirection Direction)
{
	return 0;
}

float UARCharacterAttributes::PostModifyAttribute(const FGAEvalData& AttributeMod)
{
	SCOPE_CYCLE_COUNTER(STAT_PostModifyAttribute);

	FName attribute = AttributeMod.Attribute.AttributeName;
	ARCharacterAttributes_eventInternalPostModifyAttribute_Parms params;
	params.AttributeMod = AttributeMod;
	FString prefix = "PostAttribute_";
	prefix.Append(attribute.ToString());

	TWeakObjectPtr<UFunction> NativeFunc = PostModifyAttributeFunctions.FindRef(attribute);// GetClass()->FindFunctionByName(*prefix);

	if (NativeFunc.IsValid())
	{
		ProcessEvent(NativeFunc.Get(), &params);
		return params.ReturnValue;
	}
	return 0;
}


float UARCharacterAttributes::PostAttribute_Damage(const FGAEvalData& AttributeMod)
{
	Damage = (Damage + DamageBonus.GetAdditiveBonus() - DamageBonus.GetSubtractBonus());
	Damage = Damage + (Damage * DamageBonus.GetMultiplyBonus()) / DamageBonus.GetDivideBonus();
	Health.Subtract(Damage);
	float finalDamage = Damage;// Damage;
	Damage = 0;

	if (Health.GetCurrentValue() <= 0)
	{
		if (OwningAttributeComp)
		{
			//not best solution
			//but Damage system is so tighly ingrained into
			//engine, that we can just as well take advantage of build in events
			//for some stuff.
			FDamageEvent goAway;
			OwningAttributeComp->GetOwner()->TakeDamage(finalDamage, goAway, nullptr, nullptr);
		}
		//handle death/
	}
	return finalDamage;
}
float UARCharacterAttributes::PostAttribute_FireDamage(const FGAEvalData& AttributeMod)
{
	float AddtiveBonus = DamageBonus.GetAdditiveBonus() + FireDamageBonus.GetAdditiveBonus();
	float SubtractBonus = DamageBonus.GetSubtractBonus() + FireDamageBonus.GetSubtractBonus();
	float MultiplyBonus = DamageBonus.GetMultiplyBonus() + FireDamageBonus.GetMultiplyBonus();
	float DivideBonus = DamageBonus.GetDivideBonus() + FireDamageBonus.GetDivideBonus();
	FireDamage = (FireDamage + AddtiveBonus - SubtractBonus);
	FireDamage = (FireDamage * MultiplyBonus) / DivideBonus;
	Health.Subtract(FireDamage);
	float finalDamage = FireDamage;// FireDamage;
	FireDamage = 0;

	if (Health.GetCurrentValue() <= 0)
	{
		if (OwningAttributeComp)
		{
			//not best solution
			//but Damage system is so tighly ingrained into
			//engine, that we can just as well take advantage of build in events
			//for some stuff.
			FDamageEvent goAway;
			OwningAttributeComp->GetOwner()->TakeDamage(finalDamage, goAway, nullptr, nullptr);
		}
		//handle death/
	}
	return finalDamage;
}
float UARCharacterAttributes::PostAttribute_Heal(const FGAEvalData& AttributeMod)
{
	//Health.Add(Heal);
	Heal = 0;
	return Heal;
}
float UARCharacterAttributes::PostAttribute_LifeStealDamage(const FGAEvalData& AttributeMod)
{
	return 0;
}

float UARCharacterAttributes::PostAttribute_HealthBakPrecentageReduction(const FGAEvalData& AttributeMod)
{
	return 0;
}

float UARCharacterAttributes::PostAttribute_Magic(const FGAEvalData& AttributeMod)
{
	float finalValue = 0;
	//finalValue = Magic.GetCurrentValue();
	//finalValue = (finalValue - 10) / 2;
	//MagicMod.SetBaseValue(finalValue);
	return finalValue;
}


void UARCharacterAttributes::CalculateOutgoingAttributeMods()
{
	SCOPE_CYCLE_COUNTER(STAT_OutgoingAttribute);
}
void UARCharacterAttributes::CalculateIncomingAttributeMods()
{
	SCOPE_CYCLE_COUNTER(STAT_IncomingAttribute);
	//FName Tag = AttributeModIn.AttributeTag.GetTagName();
	//FString nam = "Incoming_";
	//ARCharacterAttributes_eventInternalIncomingAttributeMod_Parms params;
	//params.AttributeModIn = AttributeModIn;
	//Tag = *Tag.ToString().Replace(TEXT("."), TEXT("_"));
	//nam = nam.Append(Tag.ToString());

	//UFunction* ModifyAttrFunc = GetClass()->FindFunctionByName(*nam);
	//if (ModifyAttrFunc)
	//{
	//	ProcessEvent(ModifyAttrFunc, &params);
	//}
}

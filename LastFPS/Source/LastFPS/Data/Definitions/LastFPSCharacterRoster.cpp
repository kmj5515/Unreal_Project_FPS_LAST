#include "Data/Definitions/LastFPSCharacterRoster.h"

#include "Data/Definitions/LastFPSCharacterDefinition.h"

const ULastFPSCharacterDefinition* ULastFPSCharacterRoster::GetDefinition(int32 Index) const
{
	return Characters.IsValidIndex(Index) ? Characters[Index].LoadSynchronous() : nullptr;
}

TSoftClassPtr<APawn> ULastFPSCharacterRoster::GetPawnClass(int32 Index) const
{
	const ULastFPSCharacterDefinition* Def = GetDefinition(Index);
	return Def ? Def->PawnClass : nullptr;
}

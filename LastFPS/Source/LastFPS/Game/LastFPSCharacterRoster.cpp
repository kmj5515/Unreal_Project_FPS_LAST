#include "Game/LastFPSCharacterRoster.h"

#include "Game/LastFPSCharacterDefinition.h"

const ULastFPSCharacterDefinition* ULastFPSCharacterRoster::GetDefinition(int32 Index) const
{
	return Characters.IsValidIndex(Index) ? Characters[Index] : nullptr;
}

TSubclassOf<APawn> ULastFPSCharacterRoster::GetPawnClass(int32 Index) const
{
	const ULastFPSCharacterDefinition* Def = GetDefinition(Index);
	return Def ? Def->PawnClass : nullptr;
}

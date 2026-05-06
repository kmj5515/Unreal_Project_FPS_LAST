#include "Game/LastFPSGameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

ALastFPSGameModeBase::ALastFPSGameModeBase()
{
    // 팀 로스터 초기화
    for (int32 i = 0; i < MaxTeams; ++i)
    {
        TeamRoster.Add(static_cast<ELastFPSTeam>(i), {});
    }
}

void ALastFPSGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    const ELastFPSTeam AssignedTeam = AssignTeam();

    if (AssignedTeam == ELastFPSTeam::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode: 모든 팀이 가득 찼습니다. 입장 거부."));
        return;
    }

    TeamRoster[AssignedTeam].Add(NewPlayer);

    UE_LOG(LogTemp, Log, TEXT("GameMode: %s → Team %d 배정 (현재 %d명)"),
        *NewPlayer->GetName(),
        static_cast<int32>(AssignedTeam),
        TeamRoster[AssignedTeam].Num());
}

void ALastFPSGameModeBase::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    // 퇴장한 컨트롤러를 모든 팀 로스터에서 제거
    for (auto& Pair : TeamRoster)
    {
        Pair.Value.RemoveAll([Exiting](const TWeakObjectPtr<AController>& Weak)
        {
            return !Weak.IsValid() || Weak.Get() == Exiting;
        });
    }
}

int32 ALastFPSGameModeBase::GetTeamPlayerCount(ELastFPSTeam Team) const
{
    if (const TArray<TWeakObjectPtr<AController>>* Roster = TeamRoster.Find(Team))
        return Roster->Num();
    return 0;
}

bool ALastFPSGameModeBase::IsTeamFull(ELastFPSTeam Team) const
{
    return GetTeamPlayerCount(Team) >= MaxPlayersPerTeam;
}

int32 ALastFPSGameModeBase::GetTotalConnectedPlayers() const
{
    int32 TotalCount = 0;

    for (const auto& Pair : TeamRoster)
    {
        for (const TWeakObjectPtr<AController>& Controller : Pair.Value)
        {
            if (Controller.IsValid())
            {
                ++TotalCount;
            }
        }
    }

    return TotalCount;
}

ELastFPSTeam ALastFPSGameModeBase::AssignTeam() const
{
    ELastFPSTeam BestTeam  = ELastFPSTeam::None;
    int32        BestCount = MaxPlayersPerTeam; // 이 값 미만인 팀만 후보

    for (int32 i = 0; i < MaxTeams; ++i)
    {
        const ELastFPSTeam Team  = static_cast<ELastFPSTeam>(i);
        const int32        Count = GetTeamPlayerCount(Team);

        if (Count < BestCount)
        {
            BestCount = Count;
            BestTeam  = Team;
        }
    }

    return BestTeam;
}

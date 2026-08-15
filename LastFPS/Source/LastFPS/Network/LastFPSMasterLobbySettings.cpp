#include "LastFPSMasterLobbySettings.h"
#include "LastFPSMasterLobbyTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool ULastFPSMasterLobbySettings::ResolveHostAndPort(FString& OutHost, int32& OutPort, FString& OutError) const
{
	FString Address = ServerAddress;
	OutPort = ServerPort;

	// 실행 인자가 설정값을 대신한다. 같은 빌드로 사설망과 외부망을 모두 쓰기 위한 통로다.
	if (!CommandLineOverrideKey.IsEmpty())
	{
		FString OverrideValue;
		const FString ParseKey = CommandLineOverrideKey + TEXT("=");
		if (FParse::Value(FCommandLine::Get(), *ParseKey, OverrideValue) && !OverrideValue.IsEmpty())
		{
			Address = MoveTemp(OverrideValue);
			UE_LOG(LogMasterLobby, Log, TEXT("실행 인자로 마스터 서버 주소를 덮어썼다. -%s%s"), *ParseKey, *Address);
		}
	}

	Address.TrimStartAndEndInline();
	// 사용자가 붙여 넣은 주소에 따옴표가 섞여 오는 경우가 잦아 함께 제거한다.
	Address.ReplaceInline(TEXT("\""), TEXT(""));

	if (Address.IsEmpty())
	{
		OutError = TEXT("마스터 로비 서버 주소가 비어 있다. 프로젝트 설정의 LastFPS Master Lobby > ServerAddress를 지정할 것.");
		return false;
	}

	// 포트를 포함한 주소를 받으면 호스트와 포트를 분리한다. IPv6 표기는 지원 대상이 아니다.
	int32 SeparatorIndex = INDEX_NONE;
	if (Address.FindLastChar(TEXT(':'), SeparatorIndex))
	{
		const FString PortText = Address.Mid(SeparatorIndex + 1);
		Address.LeftInline(SeparatorIndex, EAllowShrinking::No);

		const int32 ParsedPort = FCString::Atoi(*PortText);
		if (ParsedPort > 0 && ParsedPort <= 65535)
		{
			OutPort = ParsedPort;
		}
	}

	Address.TrimStartAndEndInline();
	if (Address.IsEmpty())
	{
		OutError = TEXT("마스터 로비 서버 주소에서 호스트를 찾을 수 없다.");
		return false;
	}

	OutHost = Address;
	return true;
}

bool ULastFPSMasterLobbySettings::BuildConnectUrl(FString& OutUrl, FString& OutError) const
{
	FString Host;
	int32 Port = 0;
	if (!ResolveHostAndPort(Host, Port, OutError))
	{
		return false;
	}

	if (Port <= 0 || Port > 65535)
	{
		OutError = FString::Printf(TEXT("마스터 로비 서버 포트가 유효하지 않다. Port=%d"), Port);
		return false;
	}

	OutUrl = FString::Printf(TEXT("%s:%d"), *Host, Port);
	return true;
}

bool ULastFPSMasterLobbySettings::BuildBeaconUrl(FString& OutUrl, FString& OutError) const
{
	FString Host;
	int32 GamePort = 0;
	if (!ResolveHostAndPort(Host, GamePort, OutError))
	{
		return false;
	}

	if (BeaconPort <= 0 || BeaconPort > 65535)
	{
		OutError = FString::Printf(TEXT("Beacon 포트가 유효하지 않다. Port=%d"), BeaconPort);
		return false;
	}

	// 게임 주소와 같은 호스트를 쓰되 포트만 Beacon 포트로 바꾼다.
	OutUrl = FString::Printf(TEXT("%s:%d"), *Host, BeaconPort);
	return true;
}

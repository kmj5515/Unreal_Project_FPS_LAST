#pragma once

#include "Math/Color.h"

/** WBP_HUD Progress Bar 기본 색 (EditDefaultsOnly로 위젯에서 덮어쓸 수 있음) */
namespace LastFPSHUDStyle
{
    inline constexpr float LowResourceRatio = 0.25f;

    inline FLinearColor GaugeBackground() { return FLinearColor(0.051f, 0.051f, 0.051f, 0.65f); }

    inline FLinearColor HealthFill()     { return FLinearColor(0.239f, 0.800f, 0.518f, 1.f); }  // #3DDC84
    inline FLinearColor HealthLowFill()  { return FLinearColor(0.906f, 0.298f, 0.235f, 1.f); }  // #E74C3C

    inline FLinearColor StaminaFill()    { return FLinearColor(0.365f, 0.678f, 0.886f, 1.f); }  // #5DADE2
    inline FLinearColor StaminaLowFill() { return FLinearColor(0.953f, 0.612f, 0.071f, 1.f); }  // #F39C12

    inline FLinearColor UltimateFill()   { return FLinearColor(0.733f, 0.525f, 0.988f, 1.f); }  // #BB86FC
    inline FLinearColor UltimateReady()  { return FLinearColor(1.000f, 0.843f, 0.000f, 1.f); }  // #FFD700

    inline FLinearColor HeatFill()       { return FLinearColor(0.953f, 0.612f, 0.071f, 1.f); }  // #F39C12
    inline FLinearColor HeatOverheated() { return FLinearColor(1.000f, 0.267f, 0.267f, 1.f); }  // #FF4444

    inline FLinearColor KillFeedKiller()     { return FLinearColor(0.945f, 0.769f, 0.059f, 1.f); }  // #F1C40F
    inline FLinearColor KillFeedVictim()     { return FLinearColor(0.906f, 0.298f, 0.235f, 1.f); }  // #E74C3C
    inline FLinearColor KillFeedSeparator()   { return FLinearColor(0.584f, 0.647f, 0.651f, 1.f); }  // #95A5A6
    inline FLinearColor KillFeedLocalPlayer(){ return FLinearColor(0.365f, 0.678f, 0.886f, 1.f); }  // #5DADE2
}

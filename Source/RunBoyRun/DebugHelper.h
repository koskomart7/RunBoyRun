#pragma once

namespace Debug
{
	// InKey의 -1 값은 여러 메시지를 인쇄할 때 최신 메세지가 맨 위에 출력되도록 한다
	static void Print(const FString& Msg, const FColor& Color = FColor::MakeRandomColor(), int32 InKey = -1)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, 6.f, Color, Msg);
		}

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
	}
}
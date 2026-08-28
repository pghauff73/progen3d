#pragma once

enum class LSystemSymbol
{
	Forward,
	TurnPositive,
	TurnNegative,
	PushState,
	PopState
};

inline const char *lSystemSymbolName(LSystemSymbol symbol)
{
	switch (symbol) {
	case LSystemSymbol::Forward: return "F";
	case LSystemSymbol::TurnPositive: return "Plus";
	case LSystemSymbol::TurnNegative: return "Minus";
	case LSystemSymbol::PushState: return "Push";
	case LSystemSymbol::PopState: return "Pop";
	}
	return "Unknown";
}

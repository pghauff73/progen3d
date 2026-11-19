#pragma once

#include <string>

struct PinRollerMember {
	std::string pinInterface, rollerInterface, materialId;
	double metresPerUnit = 1;
	double breadth = 0, depth = 0, wall = 0; // Local scene units; zero wall denotes solid.
	double modulusPa = 0, densityKgM3 = 0;
	double lineLoadNPerM = 0, distributedPayloadKg = 0, centreLoadN = 0;
};

struct MemberServiceResponse {
	double spanM = 0, areaM2 = 0, secondMomentM4 = 0, massKg = 0;
	double flexuralRigidityNm2 = 0, totalLineLoadNPerM = 0, centreLoadN = 0;
	double reactionPerSeatN = 0, pinPressurePa = 0, rollerPressurePa = 0;
	double peakMomentNm = 0, bendingStressPa = 0, midspanDeflectionM = 0;

	// Downward displacement between the two supports; station ranges from 0 to 1.
	double deflectionM(double station) const;
};

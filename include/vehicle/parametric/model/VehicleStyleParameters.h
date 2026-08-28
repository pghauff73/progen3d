#pragma once

class VehicleStyleParameters
{
public:
	VehicleStyleParameters(
		double roof_scale,
		double body_flare,
		double cabin_front_factor,
		double cabin_rear_factor,
		double tail_taper,
		double nose_taper,
		double spoiler_scale,
		double splitter_scale,
		double grille_scale,
		bool crossover_cladding)
		: roof_scale_(roof_scale),
		  body_flare_(body_flare),
		  cabin_front_factor_(cabin_front_factor),
		  cabin_rear_factor_(cabin_rear_factor),
		  tail_taper_(tail_taper),
		  nose_taper_(nose_taper),
		  spoiler_scale_(spoiler_scale),
		  splitter_scale_(splitter_scale),
		  grille_scale_(grille_scale),
		  crossover_cladding_(crossover_cladding)
	{
	}

	double roofScale() const { return roof_scale_; }
	double bodyFlare() const { return body_flare_; }
	double cabinFrontFactor() const { return cabin_front_factor_; }
	double cabinRearFactor() const { return cabin_rear_factor_; }
	double tailTaper() const { return tail_taper_; }
	double noseTaper() const { return nose_taper_; }
	double spoilerScale() const { return spoiler_scale_; }
	double splitterScale() const { return splitter_scale_; }
	double grilleScale() const { return grille_scale_; }
	bool hasCrossoverCladding() const { return crossover_cladding_; }

private:
	double roof_scale_ = 1.0;
	double body_flare_ = 0.0;
	double cabin_front_factor_ = 1.0;
	double cabin_rear_factor_ = 1.0;
	double tail_taper_ = 1.0;
	double nose_taper_ = 1.0;
	double spoiler_scale_ = 1.0;
	double splitter_scale_ = 1.0;
	double grille_scale_ = 1.0;
	bool crossover_cladding_ = false;
};

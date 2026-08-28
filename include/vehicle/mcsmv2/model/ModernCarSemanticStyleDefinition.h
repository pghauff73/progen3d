#pragma once

class ModernCarSemanticStyleDefinition
{
public:
	ModernCarSemanticStyleDefinition(
		double roof_scale,
		double greenhouse_front_factor,
		double greenhouse_rear_factor,
		double nose_taper,
		double tail_taper,
		double hood_wedge,
		double roof_crown,
		double tumblehome,
		double belt_rise,
		double shoulder_strength,
		double front_fender_amplitude,
		double rear_haunch_amplitude,
		double rocker_tuck,
		double door_scallop,
		double spoiler_scale,
		double splitter_scale,
		double grille_scale)
		: roof_scale_(roof_scale),
		  greenhouse_front_factor_(greenhouse_front_factor),
		  greenhouse_rear_factor_(greenhouse_rear_factor),
		  nose_taper_(nose_taper),
		  tail_taper_(tail_taper),
		  hood_wedge_(hood_wedge),
		  roof_crown_(roof_crown),
		  tumblehome_(tumblehome),
		  belt_rise_(belt_rise),
		  shoulder_strength_(shoulder_strength),
		  front_fender_amplitude_(front_fender_amplitude),
		  rear_haunch_amplitude_(rear_haunch_amplitude),
		  rocker_tuck_(rocker_tuck),
		  door_scallop_(door_scallop),
		  spoiler_scale_(spoiler_scale),
		  splitter_scale_(splitter_scale),
		  grille_scale_(grille_scale)
	{
	}

	double roofScale() const { return roof_scale_; }
	double greenhouseFrontFactor() const { return greenhouse_front_factor_; }
	double greenhouseRearFactor() const { return greenhouse_rear_factor_; }
	double noseTaper() const { return nose_taper_; }
	double tailTaper() const { return tail_taper_; }
	double hoodWedge() const { return hood_wedge_; }
	double roofCrown() const { return roof_crown_; }
	double tumblehome() const { return tumblehome_; }
	double beltRise() const { return belt_rise_; }
	double shoulderStrength() const { return shoulder_strength_; }
	double frontFenderAmplitude() const { return front_fender_amplitude_; }
	double rearHaunchAmplitude() const { return rear_haunch_amplitude_; }
	double rockerTuck() const { return rocker_tuck_; }
	double doorScallop() const { return door_scallop_; }
	double spoilerScale() const { return spoiler_scale_; }
	double splitterScale() const { return splitter_scale_; }
	double grilleScale() const { return grille_scale_; }

private:
	double roof_scale_ = 0.0;
	double greenhouse_front_factor_ = 0.0;
	double greenhouse_rear_factor_ = 0.0;
	double nose_taper_ = 0.0;
	double tail_taper_ = 0.0;
	double hood_wedge_ = 0.0;
	double roof_crown_ = 0.0;
	double tumblehome_ = 0.0;
	double belt_rise_ = 0.0;
	double shoulder_strength_ = 0.0;
	double front_fender_amplitude_ = 0.0;
	double rear_haunch_amplitude_ = 0.0;
	double rocker_tuck_ = 0.0;
	double door_scallop_ = 0.0;
	double spoiler_scale_ = 0.0;
	double splitter_scale_ = 0.0;
	double grille_scale_ = 0.0;
};

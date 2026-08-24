#pragma once

enum class VehicleFrontDesignLanguage
{
	Minimal,
	Technical,
	Sporting
};

enum class VehicleRearDesignLanguage
{
	Notchback,
	Fastback,
	KammTail
};

class VehicleStyleState
{
public:
	VehicleStyleState(
		float stance,
		float greenhouse_ratio,
		float wheel_body_ratio,
		float beltline_slope,
		float shoulder_strength,
		float hood_height_ratio,
		float roof_arch,
		float rear_haunch,
		float surface_tension,
		VehicleFrontDesignLanguage front_design,
		VehicleRearDesignLanguage rear_design)
		: stance_(stance),
		  greenhouse_ratio_(greenhouse_ratio),
		  wheel_body_ratio_(wheel_body_ratio),
		  beltline_slope_(beltline_slope),
		  shoulder_strength_(shoulder_strength),
		  hood_height_ratio_(hood_height_ratio),
		  roof_arch_(roof_arch),
		  rear_haunch_(rear_haunch),
		  surface_tension_(surface_tension),
		  front_design_(front_design),
		  rear_design_(rear_design)
	{
	}

	float stance() const { return stance_; }
	float greenhouseRatio() const { return greenhouse_ratio_; }
	float wheelBodyRatio() const { return wheel_body_ratio_; }
	float beltlineSlope() const { return beltline_slope_; }
	float shoulderStrength() const { return shoulder_strength_; }
	float hoodHeightRatio() const { return hood_height_ratio_; }
	float roofArch() const { return roof_arch_; }
	float rearHaunch() const { return rear_haunch_; }
	float surfaceTension() const { return surface_tension_; }
	VehicleFrontDesignLanguage frontDesign() const { return front_design_; }
	VehicleRearDesignLanguage rearDesign() const { return rear_design_; }

private:
	float stance_ = 0.0f;
	float greenhouse_ratio_ = 0.0f;
	float wheel_body_ratio_ = 0.0f;
	float beltline_slope_ = 0.0f;
	float shoulder_strength_ = 0.0f;
	float hood_height_ratio_ = 0.0f;
	float roof_arch_ = 0.0f;
	float rear_haunch_ = 0.0f;
	float surface_tension_ = 0.0f;
	VehicleFrontDesignLanguage front_design_ = VehicleFrontDesignLanguage::Minimal;
	VehicleRearDesignLanguage rear_design_ = VehicleRearDesignLanguage::Fastback;
};

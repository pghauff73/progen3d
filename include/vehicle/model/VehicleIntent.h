#pragma once

enum class VehicleBodyType
{
	Hatchback,
	Sedan,
	Fastback,
	Wagon,
	SportUtility,
	Coupe
};

enum class VehiclePowertrainType
{
	BatteryElectric,
	InternalCombustion
};

enum class VehicleDriveLayout
{
	FrontWheelDrive,
	RearWheelDrive,
	AllWheelDrive
};

enum class VehiclePerformanceClass
{
	Economy,
	Standard,
	Sport,
	Luxury,
	SportLuxury
};

enum class VehicleSizeClass
{
	Compact,
	Medium,
	Large
};

class VehicleIntent
{
public:
	VehicleIntent(
		VehicleBodyType body_type,
		VehiclePowertrainType powertrain_type,
		VehicleDriveLayout drive_layout,
		int door_count,
		int seat_count,
		VehiclePerformanceClass performance_class,
		VehicleSizeClass size_class)
		: body_type_(body_type),
		  powertrain_type_(powertrain_type),
		  drive_layout_(drive_layout),
		  door_count_(door_count),
		  seat_count_(seat_count),
		  performance_class_(performance_class),
		  size_class_(size_class)
	{
	}

	VehicleBodyType bodyType() const { return body_type_; }
	VehiclePowertrainType powertrainType() const { return powertrain_type_; }
	VehicleDriveLayout driveLayout() const { return drive_layout_; }
	int doorCount() const { return door_count_; }
	int seatCount() const { return seat_count_; }
	VehiclePerformanceClass performanceClass() const { return performance_class_; }
	VehicleSizeClass sizeClass() const { return size_class_; }

private:
	VehicleBodyType body_type_ = VehicleBodyType::Fastback;
	VehiclePowertrainType powertrain_type_ = VehiclePowertrainType::BatteryElectric;
	VehicleDriveLayout drive_layout_ = VehicleDriveLayout::AllWheelDrive;
	int door_count_ = 4;
	int seat_count_ = 5;
	VehiclePerformanceClass performance_class_ = VehiclePerformanceClass::SportLuxury;
	VehicleSizeClass size_class_ = VehicleSizeClass::Medium;
};

#pragma once

#include <cstddef>

class SmallModernBuildingModelConstructionMetrics {
public:
	double normalizationMilliseconds() const { return normalization_milliseconds_; }
	double validationMilliseconds() const { return validation_milliseconds_; }
	double requirementEvaluationMilliseconds() const
	{
		return requirement_evaluation_milliseconds_;
	}
	double hashingMilliseconds() const { return hashing_milliseconds_; }
	double totalMilliseconds() const { return total_milliseconds_; }
	std::size_t peakRecordCount() const { return peak_record_count_; }

	void setNormalizationMilliseconds(double value) { normalization_milliseconds_ = value; }
	void setValidationMilliseconds(double value) { validation_milliseconds_ = value; }
	void setRequirementEvaluationMilliseconds(double value)
	{
		requirement_evaluation_milliseconds_ = value;
	}
	void setHashingMilliseconds(double value) { hashing_milliseconds_ = value; }
	void setTotalMilliseconds(double value) { total_milliseconds_ = value; }
	void setPeakRecordCount(std::size_t value) { peak_record_count_ = value; }

private:
	double normalization_milliseconds_ = 0.0;
	double validation_milliseconds_ = 0.0;
	double requirement_evaluation_milliseconds_ = 0.0;
	double hashing_milliseconds_ = 0.0;
	double total_milliseconds_ = 0.0;
	std::size_t peak_record_count_ = 0;
};

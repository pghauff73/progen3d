#pragma once

class RigidTransformValidationReport
{
public:
	RigidTransformValidationReport(
		bool finite,
		double maximum_orthogonality_error,
		double determinant,
		double homogeneous_row_error,
		bool passed)
		: finite_(finite),
		  maximum_orthogonality_error_(maximum_orthogonality_error),
		  determinant_(determinant),
		  homogeneous_row_error_(homogeneous_row_error),
		  passed_(passed)
	{
	}

	bool finite() const { return finite_; }
	double maximumOrthogonalityError() const
	{
		return maximum_orthogonality_error_;
	}
	double determinant() const { return determinant_; }
	double homogeneousRowError() const { return homogeneous_row_error_; }
	bool passed() const { return passed_; }

private:
	bool finite_ = false;
	double maximum_orthogonality_error_ = 0.0;
	double determinant_ = 0.0;
	double homogeneous_row_error_ = 0.0;
	bool passed_ = false;
};

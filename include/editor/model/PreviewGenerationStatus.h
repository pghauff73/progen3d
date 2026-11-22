#pragma once

#include <string>
#include <utility>

// Records a rejected scene request independently of the last usable snapshot.
// A preview can remain visible after failure without being presented as current.
class PreviewGenerationStatus
{
public:
	void reject(double evaluation_time, std::string reason)
	{
		rejected_ = true;
		rejected_time_ = evaluation_time;
		reason_ = std::move(reason);
	}

	void accept()
	{
		rejected_ = false;
		reason_.clear();
	}

	bool hasRejectedRequest() const { return rejected_; }
	double rejectedTime() const { return rejected_time_; }
	const std::string &reason() const { return reason_; }

private:
	bool rejected_ = false;
	double rejected_time_ = 0.0;
	std::string reason_;
};

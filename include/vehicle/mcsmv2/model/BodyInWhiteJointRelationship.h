#pragma once

#include <string>
#include <utility>

class BodyInWhiteJointRelationship
{
public:
	BodyInWhiteJointRelationship(
		std::string first_member_identifier,
		std::string second_member_identifier,
		std::string joining_method,
		std::string shared_datum_identifier)
		: first_member_identifier_(std::move(first_member_identifier)),
		  second_member_identifier_(std::move(second_member_identifier)),
		  joining_method_(std::move(joining_method)),
		  shared_datum_identifier_(std::move(shared_datum_identifier))
	{
	}

	const std::string &firstMemberIdentifier() const
	{
		return first_member_identifier_;
	}
	const std::string &secondMemberIdentifier() const
	{
		return second_member_identifier_;
	}
	const std::string &joiningMethod() const { return joining_method_; }
	const std::string &sharedDatumIdentifier() const
	{
		return shared_datum_identifier_;
	}

private:
	std::string first_member_identifier_;
	std::string second_member_identifier_;
	std::string joining_method_;
	std::string shared_datum_identifier_;
};

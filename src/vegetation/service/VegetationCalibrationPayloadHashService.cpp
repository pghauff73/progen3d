#include "vegetation/service/VegetationCalibrationPayloadHashService.h"

#include "vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.h"

#include <openssl/evp.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

std::string
VegetationCalibrationPayloadHashService::calculateCanonicalPayloadSha256(
	const VegetationCalibrationEvidenceBundle &bundle) const
{
	const std::string payload =
		VegetationCalibrationEvidenceBundleSerializationService()
			.canonicalPayloadJson(bundle);
	std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
	unsigned int digest_size = 0u;
	if (EVP_Digest(
		    payload.data(), payload.size(), digest.data(), &digest_size,
		    EVP_sha256(), nullptr) != 1 ||
	    digest_size != 32u) {
		throw std::runtime_error(
			"Unable to calculate vegetation calibration payload SHA-256.");
	}
	std::ostringstream hexadecimal;
	hexadecimal << std::hex << std::setfill('0');
	for (unsigned int index = 0u; index < digest_size; ++index) {
		hexadecimal << std::setw(2) << static_cast<unsigned int>(digest[index]);
	}
	return hexadecimal.str();
}

VegetationCalibrationEvidenceBundle
VegetationCalibrationPayloadHashService::attachCanonicalPayloadSha256(
	const VegetationCalibrationEvidenceBundle &bundle) const
{
	return bundle.withPayloadSha256(calculateCanonicalPayloadSha256(bundle));
}

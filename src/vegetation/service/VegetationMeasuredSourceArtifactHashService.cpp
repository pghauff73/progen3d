#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"

#include <openssl/evp.h>

#include <iomanip>
#include <sstream>
#include <string>

std::string VegetationMeasuredSourceArtifactHashService::calculatePayloadSha256(
	const VegetationMeasuredSourceArtifact &artifact) const
{
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digest_length = 0u;
	EVP_MD_CTX *context = EVP_MD_CTX_new();
	if (context == nullptr) return std::string();
	const std::string &payload = artifact.sourcePayload();
	const bool succeeded =
		EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
		EVP_DigestUpdate(context, payload.data(), payload.size()) == 1 &&
		EVP_DigestFinal_ex(context, digest, &digest_length) == 1;
	EVP_MD_CTX_free(context);
	if (!succeeded) return std::string();

	std::ostringstream stream;
	stream << std::hex << std::setfill('0');
	for (unsigned int index = 0u; index < digest_length; ++index) {
		stream << std::setw(2) << static_cast<unsigned int>(digest[index]);
	}
	return stream.str();
}

VegetationMeasuredSourceArtifact
VegetationMeasuredSourceArtifactHashService::attachPayloadSha256(
	const VegetationMeasuredSourceArtifact &artifact) const
{
	return artifact.withPayloadSha256(calculatePayloadSha256(artifact));
}

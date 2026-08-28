#pragma once

#include <string>
#include <utility>
#include <vector>

class ParametricSourceFileRecord
{
public:
	ParametricSourceFileRecord(std::string path, std::string sha256)
		: path_(std::move(path)), sha256_(std::move(sha256))
	{
	}

	const std::string &path() const { return path_; }
	const std::string &sha256() const { return sha256_; }

private:
	std::string path_;
	std::string sha256_;
};

class ParametricModelSourceManifest
{
public:
	ParametricModelSourceManifest(
		std::string schema,
		std::string generator_identifier,
		std::string python_version,
		std::vector<ParametricSourceFileRecord> files,
		std::string deterministic_hash)
		: schema_(std::move(schema)),
		  generator_identifier_(std::move(generator_identifier)),
		  python_version_(std::move(python_version)),
		  files_(std::move(files)),
		  deterministic_hash_(std::move(deterministic_hash))
	{
	}

	const std::string &schema() const { return schema_; }
	const std::string &generatorIdentifier() const { return generator_identifier_; }
	const std::string &pythonVersion() const { return python_version_; }
	const std::vector<ParametricSourceFileRecord> &files() const { return files_; }
	const std::string &deterministicHash() const { return deterministic_hash_; }

private:
	std::string schema_;
	std::string generator_identifier_;
	std::string python_version_;
	std::vector<ParametricSourceFileRecord> files_;
	std::string deterministic_hash_;
};

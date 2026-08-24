#pragma once

#include "Mesh.h"

#include <fstream>
#include <string>

class MeshExportWriter
{
public:
    virtual ~MeshExportWriter() = default;
    virtual bool writeMesh(const std::string &filename, const Mesh &mesh) = 0;
};

class PlyMeshExportWriter : public MeshExportWriter
{
public:
    bool writeMesh(const std::string &filename, const Mesh &mesh) override;

protected:
    virtual void writeHeader(std::ofstream &fout, const Mesh &mesh);
    virtual void writeVertices(std::ofstream &fout, const Mesh &mesh);
    virtual void writeFaces(std::ofstream &fout, const Mesh &mesh);
};

class BinaryLittleEndianPlyMeshWriter : public PlyMeshExportWriter
{
public:
    bool writeMesh(const std::string &filename, const Mesh &mesh) override;

protected:
    void writeHeader(std::ofstream &fout, const Mesh &mesh) override;
    void writeVertices(std::ofstream &fout, const Mesh &mesh) override;
    void writeFaces(std::ofstream &fout, const Mesh &mesh) override;
};

class PLYWriter
{
public:
    static bool writeMesh(const std::string &filename, const Mesh &mesh);
};

MeshExportWriter &default_ply_mesh_writer();

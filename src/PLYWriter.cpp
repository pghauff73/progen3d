#include "PLYWriter.h"

namespace {

BinaryLittleEndianPlyMeshWriter g_binary_little_endian_ply_writer;

}

bool PlyMeshExportWriter::writeMesh(const std::string &filename, const Mesh &mesh)
{
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        return false;
    }
    writeHeader(fout, mesh);
    writeVertices(fout, mesh);
    writeFaces(fout, mesh);
    return static_cast<bool>(fout);
}

void PlyMeshExportWriter::writeHeader(std::ofstream &fout, const Mesh &mesh)
{
    fout << "ply\n";
    fout << "format ascii 1.0\n";
    fout << "element vertex " << mesh.vertices.size() << "\n";
    fout << "property float x\n";
    fout << "property float y\n";
    fout << "property float z\n";
    fout << "element face " << mesh.faces.size() << "\n";
    fout << "property list uchar int vertex_index\n";
    fout << "end_header\n";
}

void PlyMeshExportWriter::writeVertices(std::ofstream &fout, const Mesh &mesh)
{
    for (const glm::vec3 &vertex : mesh.vertices) {
        fout << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';
    }
}

void PlyMeshExportWriter::writeFaces(std::ofstream &fout, const Mesh &mesh)
{
    for (const glm::ivec3 &indices : mesh.faces) {
        fout << "3 " << indices[0] << ' ' << indices[1] << ' ' << indices[2] << '\n';
    }
}

bool BinaryLittleEndianPlyMeshWriter::writeMesh(const std::string &filename, const Mesh &mesh)
{
    std::ofstream fout(filename, std::ios::binary);
    if (!fout.is_open()) {
        return false;
    }
    writeHeader(fout, mesh);
    writeVertices(fout, mesh);
    writeFaces(fout, mesh);
    return static_cast<bool>(fout);
}

void BinaryLittleEndianPlyMeshWriter::writeHeader(std::ofstream &fout, const Mesh &mesh)
{
    fout << "ply\n";
    fout << "format binary_little_endian 1.0\n";
    fout << "element vertex " << mesh.vertices.size() << "\n";
    fout << "property float x\n";
    fout << "property float y\n";
    fout << "property float z\n";
    fout << "element face " << mesh.faces.size() << "\n";
    fout << "property list uchar int vertex_index\n";
    fout << "end_header\n";
}

void BinaryLittleEndianPlyMeshWriter::writeVertices(std::ofstream &fout, const Mesh &mesh)
{
    for (const glm::vec3 &vertex : mesh.vertices) {
        fout.write(reinterpret_cast<const char *>(&vertex.x), sizeof(float));
        fout.write(reinterpret_cast<const char *>(&vertex.y), sizeof(float));
        fout.write(reinterpret_cast<const char *>(&vertex.z), sizeof(float));
    }
}

void BinaryLittleEndianPlyMeshWriter::writeFaces(std::ofstream &fout, const Mesh &mesh)
{
    for (const glm::ivec3 &indices : mesh.faces) {
        const unsigned char vertex_count = 3;
        const int first_index = indices[0];
        const int second_index = indices[1];
        const int third_index = indices[2];
        fout.write(reinterpret_cast<const char *>(&vertex_count), sizeof(vertex_count));
        fout.write(reinterpret_cast<const char *>(&first_index), sizeof(first_index));
        fout.write(reinterpret_cast<const char *>(&second_index), sizeof(second_index));
        fout.write(reinterpret_cast<const char *>(&third_index), sizeof(third_index));
    }
}

MeshExportWriter &default_ply_mesh_writer()
{
    return g_binary_little_endian_ply_writer;
}

bool PLYWriter::writeMesh(const std::string &filename, const Mesh &mesh)
{
    return default_ply_mesh_writer().writeMesh(filename, mesh);
}

#include "geometry/model/CurveNetworkSurface.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string &message)
{
	if (!condition) throw std::runtime_error(message);
}

// Independently retain the complete tensor summation as the numerical oracle.
// It evaluates every curve and intersection, including inactive basis terms.
class DenseCurveNetworkOracle : public Surface3D
{
public:
	explicit DenseCurveNetworkOracle(const CurveNetworkSurface &surface) : surface_(surface) {}
	bool isValid(std::string *diagnostic = nullptr) const override { return surface_.isValid(diagnostic); }
	glm::dvec3 evaluatePosition(double u, double v) const override
	{
		const auto &rows = surface_.constantUCurves();
		const auto &columns = surface_.constantVCurves();
		u = std::max(0.0, std::min(1.0, u));
		v = std::max(0.0, std::min(1.0, v));
		const auto u_basis = basis(rows.size(), u);
		const auto v_basis = basis(columns.size(), v);
		glm::dvec3 point(0.0);
		for (std::size_t i = 0; i < rows.size(); ++i) point += u_basis[i] * rows[i].evaluatePosition(v);
		for (std::size_t j = 0; j < columns.size(); ++j) point += v_basis[j] * columns[j].evaluatePosition(u);
		for (std::size_t i = 0; i < rows.size(); ++i) {
			for (std::size_t j = 0; j < columns.size(); ++j) {
				const auto intersection = .5 * (rows[i].evaluatePosition(double(j) / (columns.size() - 1)) +
					columns[j].evaluatePosition(double(i) / (rows.size() - 1)));
				point -= u_basis[i] * v_basis[j] * intersection;
			}
		}
		return point;
	}
private:
	static std::vector<double> basis(std::size_t count, double value)
	{
		std::vector<double> result(count, 0.0);
		const double scaled = value * (count - 1);
		const std::size_t cell = std::min(count - 2, static_cast<std::size_t>(std::floor(scaled)));
		result[cell + 1] = scaled - cell;
		result[cell] = 1.0 - result[cell + 1];
		return result;
	}
	const CurveNetworkSurface &surface_;
};

CurveNetworkSurface curvedGrid(std::size_t row_count, std::size_t column_count)
{
	std::vector<std::vector<glm::vec3>> grid(row_count);
	for (std::size_t i = 0; i < row_count; ++i) {
		const double u = double(i) / (row_count - 1);
		for (std::size_t j = 0; j < column_count; ++j) {
			const double v = double(j) / (column_count - 1);
			grid[i].emplace_back(u, v, .12 * std::sin(2.1 * u) * std::cos(1.7 * v) + .03 * u * v);
		}
	}
	std::vector<Curve3D> rows, columns;
	for (const auto &row : grid) rows.emplace_back(Curve3DType::Polyline, row);
	for (std::size_t j = 0; j < column_count; ++j) {
		std::vector<glm::vec3> column;
		for (const auto &row : grid) column.push_back(row[j]);
		columns.emplace_back(Curve3DType::Polyline, std::move(column));
	}
	return CurveNetworkSurface(std::move(rows), std::move(columns), 1e-7);
}

} // namespace

int main()
{
	try {
		double maximum_position_error = 0.0, maximum_normal_error = 0.0;
		std::size_t samples = 0;
		for (const auto counts : {std::pair<std::size_t, std::size_t>{2, 2}, {5, 7}, {13, 17}}) {
			const auto surface = curvedGrid(counts.first, counts.second);
			std::string diagnostic;
			require(surface.isValid(&diagnostic), diagnostic);
			const DenseCurveNetworkOracle oracle(surface);
			// Include exact knots, one-sided boundaries and points on both sides
			// of knots, where derivative samples activate different basis pairs.
			std::vector<double> us{-.1, 0., .000001, .139, .371, .678, .999999, 1., 1.1};
			std::vector<double> vs{-.1, 0., .000001, .173, .503, .829, .999999, 1., 1.1};
			for (std::size_t i = 0; i < counts.first; ++i) us.push_back(double(i) / (counts.first - 1));
			for (std::size_t j = 0; j < counts.second; ++j) vs.push_back(double(j) / (counts.second - 1));
			for (double u : us) for (double v : vs) {
				maximum_position_error = std::max(maximum_position_error,
					glm::length(surface.evaluatePosition(u, v) - oracle.evaluatePosition(u, v)));
				if (u >= 0 && u <= 1 && v >= 0 && v <= 1) {
					maximum_normal_error = std::max(maximum_normal_error,
						glm::length(surface.evaluateNormal(u, v) - oracle.evaluateNormal(u, v)));
				}
				++samples;
			}
		}
		require(maximum_position_error == 0.0 && maximum_normal_error == 0.0,
			"Skipping inactive basis terms changed surface positions or normals.");
		std::cout << "PASS CurveNetworkSurface dense oracle: " << samples
			<< " samples; position error=" << maximum_position_error
			<< "; normal error=" << maximum_normal_error << '\n';
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "FAIL CurveNetworkSurface: " << error.what() << '\n';
		return 1;
	}
}

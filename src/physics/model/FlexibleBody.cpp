#include "physics/model/FlexibleBody.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;

bool inRange(double value, double low, double high) {
    return std::isfinite(value) && value >= low && value <= high;
}
bool gridIndex(double value) {
    return inRange(value, 0, 128) && value == std::floor(value);
}
double turnAngle(glm::dvec3 incoming, glm::dvec3 outgoing) {
    return std::atan2(glm::length(glm::cross(incoming, outgoing)), glm::dot(incoming, outgoing));
}
}

std::optional<FlexibleTopology> FlexibleBodySpecification::topologyOf(const std::string &type) {
    if (type == "FlexibleWire") return FlexibleTopology::Wire;
    if (type == "FlexibleBelt") return FlexibleTopology::Belt;
    if (type == "ElasticSheet") return FlexibleTopology::Sheet;
    return {};
}

int FlexibleBodySpecification::argumentCount(const std::string &action) {
    if (action == "FlexDimensions" || action == "FlexMaterial") return 3;
    if (action == "FlexGrid" || action == "FlexPin") return 2;
    if (action == "FlexGuideRadius" || action == "FlexBeltSpeed") return 1;
    return 0;
}

std::string FlexibleBodySpecification::configure(const std::string &action, glm::vec3 values) {
    if (action == "FlexDimensions") {
        dimensions = values;
    } else if (action == "FlexMaterial") {
        material = {values.x, values.y, values.z};
    } else if (action == "FlexGuideRadius") {
        guideRadius = values.x;
    } else if (action == "FlexBeltSpeed") {
        initialBeltSpeed = values.x;
    } else if (action == "FlexGrid" || action == "FlexPin") {
        if (!gridIndex(values.x) || !gridIndex(values.y))
            return action + " requires integer indices in [0,128].";
        if (action == "FlexGrid") grid = glm::ivec2(values);
        else pins.emplace_back(values);
    } else return "Unknown flexible object declaration.";
    return {};
}

glm::ivec2 FlexibleBodySpecification::gridFor(FlexibleTopology topology) const {
    if (grid) return *grid;
    if (topology == FlexibleTopology::Wire) return {24, 0};
    if (topology == FlexibleTopology::Belt) return {32, 1};
    return {12, 8};
}
glm::dvec3 FlexibleBodySpecification::dimensionsFor(FlexibleTopology topology) const {
    if (dimensions) return *dimensions;
    if (topology == FlexibleTopology::Wire) return {1, .006, 0};
    if (topology == FlexibleTopology::Belt) return {.6, .04, .002};
    return {1, .6, .002};
}
std::size_t FlexibleBodySpecification::particleCount(FlexibleTopology topology) const {
    const auto cells = gridFor(topology);
    const int rows = cells.x + (topology == FlexibleTopology::Belt ? 0 : 1);
    return static_cast<std::size_t>(rows) * (cells.y + 1);
}

std::string FlexibleBodySpecification::validate(FlexibleTopology topology) const {
    if (!inRange(material.youngModulus, 1, 1e13) || !inRange(material.density, .001, 1e6) ||
        !inRange(material.dampingRate, 0, 1000))
        return "FlexMaterial requires density [0.001,1e6], Young's modulus [1,1e13] and damping [0,1000].";
    const auto size = dimensionsFor(topology);
    const bool wire = topology == FlexibleTopology::Wire;
    if (!inRange(size.x, 1e-6, 1e4) || !inRange(size.y, 1e-6, 1e4) ||
        !inRange(size.z, wire ? 0 : 1e-6, 1e4))
        return "FlexDimensions requires positive metre dimensions (wire rest sag may be zero).";
    const auto cells = gridFor(topology);
    if (cells.x < 2 || cells.x > 128 || cells.y < 0 || cells.y > 32 ||
        (wire ? cells.y != 0 : cells.y == 0) || particleCount(topology) > 2048)
        return "Invalid FlexGrid: use 2..128 longitudinal cells, 0 across a wire or 1..32 across a surface, up to 2048 particles.";
    const bool belt = topology == FlexibleTopology::Belt;
    if (belt && (cells.x < 8 || cells.x % 4 != 0))
        return "FlexibleBelt requires a longitudinal grid divisible by four, at least eight.";
    if ((guideRadius && !belt) || !inRange(guideRadius.value_or(.1), 1e-6, 1e4))
        return "FlexGuideRadius requires a positive belt pitch radius.";
    if (initialBeltSpeed && (!belt || !inRange(*initialBeltSpeed,-1e3,1e3)))
        return "FlexBeltSpeed requires a belt and a finite initial circulation speed in [-1000,1000] m/s.";
    for (const auto &pin : pins) {
        if (pin.x < 0 || pin.y < 0 || pin.x > cells.x - (belt ? 1 : 0) || pin.y > cells.y)
            return "FlexPin lies outside the particle grid.";
    }
    return {};
}

FlexibleBody::FlexibleBody(FlexibleTopology topology, const FlexibleBodySpecification &spec, double scale)
    : topology_(topology), material_(spec.material), dimensions_(spec.dimensionsFor(topology) * scale),
      grid_(spec.gridFor(topology)), scale_(scale), guideRadius_(spec.guideRadius.value_or(.1) * scale),
      beltSpeed_(spec.initialBeltSpeed.value_or(0)) {
    const auto diagnostic = spec.validate(topology);
    if (!diagnostic.empty()) throw std::invalid_argument(diagnostic);
    if (!inRange(scale, 1e-6, 1e6)) throw std::invalid_argument("Invalid flexible object scale.");
    particles_.resize(spec.particleCount(topology));
    if (topology == FlexibleTopology::Wire) buildWire();
    else buildRibbon();
    for (auto &p : particles_) {
        p.position = p.restPosition;
        p.inverseMass = 1.0 / p.mass;
        mass_ += p.mass;
    }
    for (const auto &pin : spec.pins) particles_[index(pin.x, pin.y)].inverseMass = 0;
    initialVelocities_.resize(particles_.size(), glm::dvec3(0));
    previousPositions_.resize(particles_.size());
    setInitialMotion(glm::dvec3(0), glm::dvec3(0));
}
int FlexibleBody::index(int along, int across) const {
    if (topology_ == FlexibleTopology::Belt) along %= grid_.x;
    return along * (grid_.y + 1) + across;
}
void FlexibleBody::addStretch(int first, int second, double axialRigidity) {
    const double length = glm::distance(particles_[first].restPosition, particles_[second].restPosition);
    stretches_.push_back({first, second, length, length / axialRigidity});
}
void FlexibleBody::addBend(int first, int middle, int last, double flexuralRigidity) {
    const auto incoming = particles_[middle].restPosition - particles_[first].restPosition;
    const auto outgoing = particles_[last].restPosition - particles_[middle].restPosition;
    const double spacing = .5 * (glm::length(incoming) + glm::length(outgoing));
    bends_.push_back({first, middle, last, turnAngle(incoming, outgoing), spacing / flexuralRigidity});
}
void FlexibleBody::buildWire() {
    const double radius = .5 * dimensions_.y;
    const double area = pi * radius * radius;
    const double axialRigidity = material_.youngModulus * area;
    for (int i = 0; i <= grid_.x; ++i) {
        const double u = static_cast<double>(i) / grid_.x;
        particles_[i].restPosition = {dimensions_.x * u, 0, -4 * dimensions_.z * u * (1-u)};
        if (i == 0) continue;
        addStretch(i-1, i, axialRigidity);
        const double halfMass = .5 * material_.density * area * stretches_.back().restLength;
        particles_[i-1].mass += halfMass;
        particles_[i].mass += halfMass;
        if (i >= 2) addBend(i-2, i-1, i, axialRigidity * radius * radius / 4);
    }
}
glm::dvec3 FlexibleBody::ribbonPoint(int along, int across) const {
    const double y = dimensions_.y * (static_cast<double>(across) / grid_.y - .5);
    const double u = static_cast<double>(along) / grid_.x;
    if (topology_ == FlexibleTopology::Sheet) return {dimensions_.x * u, y, 0};
    // Racetrack with equal pitch radii, centres on X, axes along Y.
    const double t = 4*u;
    if (t < 1) return {dimensions_.x*t, y, guideRadius_};
    if (t < 2) return {dimensions_.x + guideRadius_*std::sin(pi*(t-1)), y, guideRadius_*std::cos(pi*(t-1))};
    if (t < 3) return {dimensions_.x*(3-t), y, -guideRadius_};
    return {-guideRadius_*std::sin(pi*(t-3)), y, -guideRadius_*std::cos(pi*(t-3))};
}
void FlexibleBody::buildRibbon() {
    const bool closed = topology_ == FlexibleTopology::Belt;
    const int rows = grid_.x + (closed ? 0 : 1);
    const double spacingY = dimensions_.y / grid_.y;
    const double thickness = dimensions_.z;
    const double membrane = material_.youngModulus * thickness;
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j <= grid_.y; ++j) particles_[index(i,j)].restPosition = ribbonPoint(i,j);
    for (int i = 0; i < rows; ++i) {
        const int next = (i+1) % rows, prev = (i+rows-1) % rows;
        const bool hasNext = closed || i < rows-1, hasPrev = closed || i > 0;
        const auto &p = particles_[index(i,0)].restPosition;
        const double forward = hasNext ? glm::distance(p,particles_[index(next,0)].restPosition) : 0;
        const double backward = hasPrev ? glm::distance(p,particles_[index(prev,0)].restPosition) : 0;
        const double tributaryX = .5*(forward+backward);
        for (int j = 0; j <= grid_.y; ++j) {
            const int a = index(i,j);
            const double tributaryY = (j == 0 || j == grid_.y) ? .5*spacingY : spacingY;
            particles_[a].mass = material_.density * thickness * tributaryX * tributaryY;
            if (hasNext) addStretch(a, index(next,j), membrane * tributaryY);
            if (hasNext && hasPrev) addBend(index(prev,j), a, index(next,j),
                                           membrane * tributaryY * thickness * thickness / 12);
            if (j == grid_.y) continue;
            addStretch(a, index(i,j+1), membrane * tributaryX);
            if (j > 0) addBend(index(i,j-1), a, index(i,j+1),
                               membrane * tributaryX * thickness * thickness / 12);
            if (hasNext) {
                // Symmetric diagonal bracing supplies an effective in-plane shear response.
                const double diagonal = std::hypot(forward, spacingY);
                const double rigidity = .25 * membrane * forward * spacingY / diagonal;
                addStretch(a, index(next,j+1), rigidity);
                addStretch(index(i,j+1), index(next,j), rigidity);
            }
        }
    }
}
void FlexibleBody::setTotalMass(double kilograms) {
    if (!(kilograms > 0 && std::isfinite(kilograms))) throw std::invalid_argument("Invalid flexible body mass.");
    const double ratio = kilograms / mass_;
    for (auto &p : particles_) {
        p.mass *= ratio;
        if (p.inverseMass > 0) p.inverseMass = 1 / p.mass;
    }
    mass_ = kilograms;
}
void FlexibleBody::setInitialMotion(glm::dvec3 linear, glm::dvec3 angular) {
    glm::dvec3 center(0);
    for (const auto &p : particles_) center += p.mass * p.restPosition;
    center /= mass_;
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        auto &p = particles_[i];
        auto velocity = linear + glm::cross(angular,p.restPosition-center);
        if (beltSpeed_ != 0) {
            const int row = static_cast<int>(i)/(grid_.y+1);
            const int column = static_cast<int>(i)%(grid_.y+1);
            const auto &prev = particles_[index((row+grid_.x-1)%grid_.x,column)].restPosition;
            const auto &next = particles_[index(row+1,column)].restPosition;
            const auto tangent = glm::normalize(next-p.restPosition) + glm::normalize(p.restPosition-prev);
            velocity += beltSpeed_ * glm::normalize(tangent);
        }
        initialVelocities_[i] = p.inverseMass > 0 ? velocity : glm::dvec3(0);
    }
    reset();
}
void FlexibleBody::reset() {
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        particles_[i].position = particles_[i].restPosition;
        particles_[i].velocity = initialVelocities_[i];
    }
}
void FlexibleBody::solveStretch(StretchConstraint &edge, double inverseStepSquared) {
    auto &a = particles_[edge.first], &b = particles_[edge.second];
    auto delta = b.position - a.position;
    const double length = glm::length(delta);
    if (length < 1e-18) return;
    const double alpha = edge.compliance * inverseStepSquared;
    const double change = (edge.restLength-length-alpha*edge.multiplier)/(a.inverseMass+b.inverseMass+alpha);
    delta *= change / length;
    a.position -= a.inverseMass*delta;
    b.position += b.inverseMass*delta;
    edge.multiplier += change;
}
void FlexibleBody::solveBend(BendConstraint &hinge, double inverseStepSquared) {
    auto &a = particles_[hinge.first], &b = particles_[hinge.middle], &c = particles_[hinge.last];
    const auto incoming = b.position-a.position, outgoing = c.position-b.position;
    const auto normal = glm::cross(incoming, outgoing);
    const double length = glm::length(normal);
    if (length <= 1e-12 * glm::length(incoming) * glm::length(outgoing)) return;
    const auto axis = normal/length;
    const auto ga = glm::cross(axis,incoming)/glm::dot(incoming,incoming);
    const auto gc = glm::cross(axis,outgoing)/glm::dot(outgoing,outgoing);
    const auto gb = -ga-gc;
    const double weight = a.inverseMass*glm::dot(ga,ga) + b.inverseMass*glm::dot(gb,gb) + c.inverseMass*glm::dot(gc,gc);
    const double alpha = hinge.compliance * inverseStepSquared;
    const double change = (hinge.restAngle-turnAngle(incoming,outgoing)-alpha*hinge.multiplier)/(weight+alpha);
    a.position += a.inverseMass*change*ga;
    b.position += b.inverseMass*change*gb;
    c.position += c.inverseMass*change*gc;
    hinge.multiplier += change;
}
void FlexibleBody::projectBeltGuides() {
    for (auto &p : particles_) {
        if (p.inverseMass == 0) continue;
        for (double x : {0.0, dimensions_.x}) {
            const glm::dvec2 radial(p.position.x-x,p.position.z);
            const double length = glm::length(radial);
            if (length >= guideRadius_) continue;
            const auto direction = length > 1e-16 ? radial/length : glm::dvec2(0,1);
            p.position.x = x + guideRadius_*direction.x;
            p.position.z = guideRadius_*direction.y;
        }
    }
}
void FlexibleBody::advance(double seconds, glm::dvec3 acceleration) {
    if (!(seconds > 0 && std::isfinite(seconds))) return;
    // Scene ticks are interactive. Limit work on a single unusually long tick.
    const int steps = static_cast<int>(std::clamp(std::ceil(seconds*240-1e-6), 1., 240.));
    const double h = seconds/steps, inverseStepSquared = 1/(h*h);
    const double retention = std::exp(-material_.dampingRate*h);
    for (int step = 0; step < steps; ++step) {
        for (std::size_t i=0; i<particles_.size(); ++i) {
            auto &p = particles_[i];
            previousPositions_[i] = p.position;
            if (p.inverseMass == 0) continue;
            p.position += h*(retention*p.velocity+h*acceleration);
        }
        for (auto &edge : stretches_) edge.multiplier = 0;
        for (auto &hinge : bends_) hinge.multiplier = 0;
        for (int pass=0; pass<4; ++pass) {
            for (auto &edge : stretches_) solveStretch(edge,inverseStepSquared);
            for (auto &hinge : bends_) solveBend(hinge,inverseStepSquared);
            if (topology_ == FlexibleTopology::Belt) projectBeltGuides();
        }
        for (std::size_t i=0; i<particles_.size(); ++i)
            particles_[i].velocity = (particles_[i].position-previousPositions_[i])/h;
    }
}

Mesh FlexibleBody::surfaceMesh() const {
    Mesh mesh;
    if (topology_ == FlexibleTopology::Wire) {
        constexpr int sides = 8;
        glm::dvec3 radial(0,1,0);
        for (int i=0; i<=grid_.x; ++i) {
            auto direction = particles_[std::min(i+1,grid_.x)].position -
                             particles_[std::max(i-1,0)].position;
            const double length = glm::length(direction);
            direction = length > 1e-16 ? direction/length : glm::dvec3(1,0,0);
            radial -= glm::dot(radial,direction)*direction;
            if (glm::length(radial) < 1e-8) {
                const auto axis = std::abs(direction.x) < .8 ? glm::dvec3(1,0,0) : glm::dvec3(0,1,0);
                radial = glm::cross(direction,axis);
            }
            radial = glm::normalize(radial);
            const auto binormal = glm::cross(direction,radial);
            for (int j=0; j<sides; ++j) {
                const double angle = 2*pi*j/sides;
                const auto offset = .5*dimensions_.y*(std::cos(angle)*radial+std::sin(angle)*binormal);
                mesh.vertices.emplace_back((particles_[i].position+offset)/scale_);
                if (i > 0) {
                    const int a = (i-1)*sides+j, b = (i-1)*sides+(j+1)%sides;
                    mesh.addQuad(a,b,b+sides,a+sides);
                }
            }
        }
        const int last = grid_.x*sides;
        for (int j=1; j<sides-1; ++j) {
            mesh.addTriangle(0,j+1,j);
            mesh.addTriangle(last,last+j,last+j+1);
        }
        return mesh;
    }
    const bool closed = topology_ == FlexibleTopology::Belt;
    const int rows = grid_.x + (closed ? 0 : 1);
    const int layer = static_cast<int>(particles_.size());
    mesh.vertices.resize(2*layer);
    for (int i=0; i<rows; ++i) {
        const int prev = closed ? (i+rows-1)%rows : std::max(i-1,0);
        const int next = closed ? (i+1)%rows : std::min(i+1,rows-1);
        for (int j=0; j<=grid_.y; ++j) {
            const auto along = particles_[index(next,j)].position-particles_[index(prev,j)].position;
            const auto across = particles_[index(i,std::min(j+1,grid_.y))].position -
                                particles_[index(i,std::max(j-1,0))].position;
            const auto normal = glm::cross(along,across);
            const double length = glm::length(normal);
            const auto offset = .5*dimensions_.z*(length > 1e-20 ? normal/length : glm::dvec3(0,0,1));
            const int a = index(i,j);
            mesh.vertices[a] = (particles_[a].position+offset)/scale_;
            mesh.vertices[a+layer] = (particles_[a].position-offset)/scale_;
            if (j == grid_.y || (!closed && i+1 == rows)) continue;
            const int b=index(next,j), c=index(next,j+1), d=index(i,j+1);
            mesh.addQuad(a,b,c,d);
            mesh.addQuad(a+layer,d+layer,c+layer,b+layer);
            if (j == 0) mesh.addQuad(a,a+layer,b+layer,b);
            if (j+1 == grid_.y) mesh.addQuad(c,c+layer,d+layer,d);
            if (!closed && i == 0) mesh.addQuad(d,d+layer,a+layer,a);
            if (!closed && i+1 == rows-1) mesh.addQuad(b,b+layer,c+layer,c);
        }
    }
    return mesh;
}

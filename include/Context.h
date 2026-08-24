#pragma once

#include "Scope.h"
#include "Mesh.h"
#include "geometry/model/AxisAlignedBounds.h"
#include "geometry/model/ResolvedPrimitiveGeometry.h"
#include "physics/model/CollisionGeometry.h"

#include <cstddef>
#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "ProGen3dGl.h"

class PrimitiveDefinition {
public:
	PrimitiveDefinition(std::string type, GLuint texId_name, bool, bool, bool);
	void draw(Scope *scope, int material_index, float alpha, float texscale);

	std::string type;
	GLuint texId;
	bool x = false;
	bool y = false;
	bool z = false;
};

class PrimitiveGeometryResolver;
class ShapeSpecification;
class CollisionParticipationPolicy;
class SpatialBuildingModel;
class SmallModernBuildingModel;
class SpatialBuildingModelConstructionContext;
class SpatialBuildingObject;
class SpatialConnection;
class SpatialConstraint;
class SpatialInterface;
class SpatialObjectIdentity;
class LightingSceneDefinition;
class SceneLight;
class LightFixtureObject;
class LightSwitch;
class LightingCircuit;
class VehicleJoint;
class VehicleJointGraph;

class SceneObjectState {
public:
	virtual ~SceneObjectState() = default;
	virtual void restoreInitialState();

	glm::vec3 position{0.0f};
	glm::vec3 initial_position{0.0f};
	glm::vec3 velocity{0.0f};
	glm::vec3 initial_velocity{0.0f};
	glm::vec3 rotational_velocity{0.0f};
	glm::vec3 initial_rotational_velocity{0.0f};
	float mass = 1.0f;
	float initial_mass = 1.0f;
	float sleep_timer = 0.0f;
	bool immovable = false;
	bool initial_immovable = false;
	bool simulation_active = true;
	bool initial_simulation_active = true;
	bool removed = false;
	bool initial_removed = false;
	bool sleeping = false;
};

class SceneRenderableState : public SceneObjectState {
public:
	void restoreInitialState() override;

	glm::vec3 previous_bounds_center{0.0f};
	int source_start_line = -1;
	int source_start_column = 0;
	int source_end_line = -1;
	int source_end_column = 0;
	int material_index = 0;
	std::string material_name;
	float alpha = 0.0f;
	float texscale = 0.125f;
	bool has_previous_render_state = false;
};

class ScenePrimitiveInstance : public SceneRenderableState {
public:
	void restoreInitialState() override;

	PrimitiveDefinition *primitive = nullptr;
	std::string type;
	glm::mat4 primary_transform{1.0f};
	glm::mat4 secondary_transform{1.0f};
	glm::mat4 previous_primary_transform{1.0f};
	glm::mat4 previous_secondary_transform{1.0f};
	glm::mat4 initial_primary_transform{1.0f};
	glm::mat4 initial_secondary_transform{1.0f};
	glm::vec3 size{1.0f};
	glm::vec3 size2{1.0f};
	std::array<glm::vec3, 3> dual_scales{glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
	std::array<glm::vec3, 3> dual_translations{glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
	std::array<glm::vec3, 3> initial_dual_scales{glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
	std::array<glm::vec3, 3> initial_dual_translations{glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
	glm::vec3 rotation_degrees{0.0f};
	glm::vec3 initial_rotation_degrees{0.0f};
	std::shared_ptr<const ResolvedPrimitiveGeometry> resolved_geometry;
	std::shared_ptr<const ShapeSpecification> shape_specification;
	mutable CollisionGeometry collision_geometry_cache;
	mutable bool collision_geometry_dirty = true;
};

struct CollisionParticle {
	glm::vec3 position{0.0f};
	glm::vec3 velocity{0.0f};
	glm::vec4 color{1.0f};
	float lifetime = 0.0f;
	float initial_lifetime = 0.0f;
	float size = 0.0f;
};

class SceneGenerationContext
{
public:
	SceneGenerationContext();
	~SceneGenerationContext();
	void pushScope();
	Scope *popScope();
	void newScope();
	Scope *getCurrentScope();
	Mesh &getScene();
	Mesh buildExportMesh(const GLfloat *vertex_data) const;
	const std::vector<std::string> &getMaterialNames() const;
	void draw();
	GLfloat *calc(const GLfloat *, int material_index, int *out_count = nullptr);
	void buildMaterialBuffers(const GLfloat *vertex_data,
	                          std::size_t material_count,
	                          std::vector<std::vector<GLfloat>> *buffers,
	                          std::vector<int> *counts) const;
	void buildInstanceBuffer(const ScenePrimitiveInstance &instance,
	                         const GLfloat *vertex_data,
	                         std::vector<GLfloat> *buffer) const;
	glm::vec3 getInstanceCenter(const ScenePrimitiveInstance &instance) const;
	PrimitiveBounds getInstanceBounds(const ScenePrimitiveInstance &instance) const;
	const CollisionGeometry &getInstanceCollisionGeometry(const ScenePrimitiveInstance &instance) const;
	PrimitiveBounds getSceneBounds() const;
	void PLY(const GLfloat *, const std::string &filename);
	void addPrimitive(std::string type,
	                  Scope *scope,
	                  const std::string &material_name,
	                  float alpha,
	                  float texscale,
	                  bool immovable = false,
	                  int source_start_line = -1,
	                  int source_start_column = 0,
	                  int source_end_line = -1,
	                  int source_end_column = 0,
	                  std::shared_ptr<const ShapeSpecification> shape_specification = {});
	bool beginSpatialObject(SpatialObjectIdentity identity,
	                        const std::string &container_object_id,
	                        CollisionParticipationPolicy collision_policy,
	                        std::string *diagnostic);
	bool addSpatialInterface(SpatialInterface interface, std::string *diagnostic);
	bool addSpatialConnection(SpatialConnection connection, std::string *diagnostic);
	bool addSpatialConstraint(std::shared_ptr<const SpatialConstraint> constraint,
	                         std::string *diagnostic);
	bool endSpatialObject(std::string *diagnostic);
	bool finalizeSpatialBuildingModel(std::string *diagnostic);
	bool hasActiveSpatialObject() const;
	std::string currentSpatialObjectId() const;
	const std::shared_ptr<const SpatialBuildingModel> &spatialBuildingModel() const;
	const std::shared_ptr<const SmallModernBuildingModel> &smallModernBuildingModel() const;
	bool addSceneLight(SceneLight light, std::string *diagnostic);
	bool addLightFixture(LightFixtureObject fixture, std::string *diagnostic);
	bool addLightFixtureWithEmitter(SceneLight light,
	                               LightFixtureObject fixture,
	                               std::string *diagnostic);
	bool addLightSwitch(LightSwitch light_switch, std::string *diagnostic);
	bool addLightingCircuit(LightingCircuit circuit, std::string *diagnostic);
	const LightingSceneDefinition &lightingSceneDefinition() const;
	bool addVehicleJoint(VehicleJoint joint, std::string *diagnostic);
	const VehicleJointGraph &vehicleJointGraph() const;
	void setPendingVelocity(const glm::vec3 &velocity);
	void setPendingRotationalVelocity(const glm::vec3 &rotational_velocity);
	void setPendingMass(float mass);
	void setPendingDensity(float density);
	void setGravity(float magnitude, const glm::vec3 &direction);
	const glm::vec3 &getGravity() const;
	const std::vector<CollisionParticle> &getCollisionParticles() const;
	void emitCollisionParticles(const glm::vec3 &contact_point,
	                            const glm::vec3 &normal,
	                            const glm::vec3 &relative_velocity,
	                            float impulse_magnitude);
	void clearPendingPrimitiveState();
	void resetSimulation();
	void stepSimulation(float delta_time);
	float getTime() const;
	void genPrimitives();

	Scope *current_scope = NULL;
	std::stack<Scope *> scopes;
	Mesh scene;
	PrimitiveDefinition *Cube = nullptr;
	PrimitiveDefinition *ProceduralShape = nullptr;
	PrimitiveDefinition *AxialProfile = nullptr;
	PrimitiveDefinition *Cylinder = nullptr;
	PrimitiveDefinition *Sphere = nullptr;
	PrimitiveDefinition *CubeX = nullptr;
	PrimitiveDefinition *CubeY = nullptr;
	PrimitiveDefinition *CubeZ = nullptr;
	std::vector<ScenePrimitiveInstance> primitive_instances;
	float time = 0.0f;

private:
	std::size_t resolveMaterialSlot(const std::string &material_name);
	glm::vec3 pending_velocity_{0.0f};
	glm::vec3 pending_rotational_velocity_{0.0f};
	float pending_mass_ = 1.0f;
	float pending_density_ = 0.0f;
	bool has_pending_density_ = false;
	glm::vec3 gravity_acceleration_{0.0f};
	std::vector<std::string> material_names_;
	std::vector<CollisionParticle> collision_particles_;
	glm::vec3 scene_bounds_min_{0.0f};
	glm::vec3 scene_bounds_max_{0.0f};
	bool has_scene_bounds_ = false;
	std::unique_ptr<PrimitiveGeometryResolver> primitive_geometry_resolver_;
	class SpatialSceneConstructionState;
	std::unique_ptr<SpatialSceneConstructionState> spatial_scene_construction_state_;
	std::unique_ptr<LightingSceneDefinition> lighting_scene_definition_;
	std::unique_ptr<VehicleJointGraph> vehicle_joint_graph_;

	void stepCollisionParticles(float delta_time);
};

using Primitive = PrimitiveDefinition;
using PrimitiveInstance = ScenePrimitiveInstance;
using Context = SceneGenerationContext;

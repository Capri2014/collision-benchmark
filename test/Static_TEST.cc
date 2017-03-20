#include <collision_benchmark/WorldManager.hh>
#include <collision_benchmark/PrimitiveShape.hh>
#include <collision_benchmark/SimpleTriMeshShape.hh>
#include <collision_benchmark/BasicTypes.hh>

#include <collision_benchmark/MeshShapeGeneratorVtk.hh>

#include <gazebo/gazebo.hh>


#include "StaticTestFramework.hh"

using collision_benchmark::Shape;
using collision_benchmark::PrimitiveShape;
using collision_benchmark::SimpleTriMeshShape;

// tolerance for values close to zero: All contacts as close to zero will be
// considered "just touching" and disagreement of engines won't be triggered.
const double zeroDepthTol = 5e-02;

// minimum agreement of engines that has to be reached, value in range [0..1].
const double minAgree = 0.999;

class StaticTest : public StaticTestFramework {};


Shape::Ptr GetSimpleTestTriangle(const std::string& modelName)
{
  std::string modelName1 = "model1";
  // create simple mesh for testing
  SimpleTriMeshShape::MeshDataPtr meshData(new SimpleTriMeshShape::MeshDataT());
  typedef SimpleTriMeshShape::Vertex Vertex;
  typedef SimpleTriMeshShape::Face Face;
  std::vector<Vertex>& vertices=meshData->GetVertices();
  std::vector<Face>& triangles=meshData->GetFaces();
  vertices.push_back(Vertex(-1,0,0));
  vertices.push_back(Vertex(0,0,-1));
  vertices.push_back(Vertex(1,0,0));
  vertices.push_back(Vertex(0,1,0));
  triangles.push_back(Face(0,1,2));
  triangles.push_back(Face(0,2,3));
  Shape::Ptr shape(new SimpleTriMeshShape(meshData, modelName1));
  return shape;
}


TEST_F(StaticTest, TwoShapesTest1)
{
  std::vector<std::string> selectedEngines;
  selectedEngines.push_back("bullet");
  selectedEngines.push_back("ode");
  selectedEngines.push_back("dart");
  // selectedEngines.push_back("simbody");

  /*std::set<std::string> engines =
    collision_benchmark::GetSupportedPhysicsEngines();
  // run test on all engines
  selectedEngines.insert(selectedEngines.end(), engines.begin(), engines.end());*/

  // Model 1
  std::string modelName1 = "model1";
  Shape::Ptr shape1(PrimitiveShape::CreateBox(2,2,2));
  // Model 2
  std::string modelName2 = "model2";
  Shape::Ptr shape2(PrimitiveShape::CreateCylinder(1,3));

  PrepareWorld(selectedEngines);
  LoadShape(shape1, modelName1);
  LoadShape(shape2, modelName2);
  const static bool interactive = true;
  const static float cellSizeFactor = 0.1;
  AaBbTest(modelName1, modelName2, cellSizeFactor, minAgree,
           zeroDepthTol, interactive);
}

TEST_F(StaticTest, CylinderAndTwoTriangles)
{
  std::vector<std::string> selectedEngines;
  selectedEngines.push_back("bullet");
  selectedEngines.push_back("ode");
  selectedEngines.push_back("dart");
  // selectedEngines.push_back("simbody");

  /*std::set<std::string> engines =
    collision_benchmark::GetSupportedPhysicsEngines();
  // run test on all engines
  selectedEngines.insert(selectedEngines.end(), engines.begin(), engines.end());*/

  // Model 1
  std::string modelName1 = "model1";
  Shape::Ptr shape1 = GetSimpleTestTriangle(modelName1);

  // Model 2
  std::string modelName2 = "model2";
  Shape::Ptr shape2(PrimitiveShape::CreateCylinder(1,3));

  PrepareWorld(selectedEngines);
  LoadShape(shape1, modelName1);
  LoadShape(shape2, modelName2);
  const static bool interactive = true;
  const static float cellSizeFactor = 0.1;
  const std::string outputPath; // ="/home/jenny/testResults";
  AaBbTest(modelName1, modelName2, cellSizeFactor, minAgree, zeroDepthTol,
           interactive, outputPath);
}

TEST_F(StaticTest, SpherePrimMesh)
{
  std::vector<std::string> selectedEngines;
  selectedEngines.push_back("bullet");
  selectedEngines.push_back("ode");
  selectedEngines.push_back("dart");
  // selectedEngines.push_back("simbody");

  /*std::set<std::string> engines =
    collision_benchmark::GetSupportedPhysicsEngines();
  // run test on all engines
  selectedEngines.insert(selectedEngines.end(), engines.begin(), engines.end());*/

  typedef SimpleTriMeshShape::MeshDataT::VertexPrecision Precision;
  collision_benchmark::MeshShapeGenerator<Precision>::Ptr generator
      (new collision_benchmark::MeshShapeGeneratorVtk<Precision>());

  double radius = 2;

  // sphere as mesh
  std::string meshName = "SphereMesh";
  SimpleTriMeshShape::MeshDataT::Ptr sphereMeshData =
    generator->MakeSphere(radius, 10, 10);
    //generator->MakeCylinder(radius, 4, 50, true);
  //sphereMeshData->Perturb(-0.2, 0.2, SimpleTriMeshShape::MeshDataT::Vertex(0,0,0),
  //                        SimpleTriMeshShape::MeshDataT::Vertex(0,1,0));
  Shape::Ptr sphereMesh(new SimpleTriMeshShape(sphereMeshData, meshName));

  // sphere as a primitive
  std::string primName = "SpherePrimitive";
  Shape::Ptr spherePrimitive(PrimitiveShape::CreateSphere(radius));

  PrepareWorld(selectedEngines);
  LoadShape(sphereMesh, meshName);
  LoadShape(spherePrimitive, primName);
  const static bool interactive = true;
  const static float cellSizeFactor = 0.1;
  const std::string outputPath; // ="/home/jenny/testResults";
  AaBbTest(meshName, primName, cellSizeFactor, minAgree, zeroDepthTol,
           interactive, outputPath);
}

int main(int argc, char**argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

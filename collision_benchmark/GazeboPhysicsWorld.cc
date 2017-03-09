/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
/**
 * Desc: PhysicsEngineWorld implementation for Gazebo which wraps a
 * gazebo::physics::World object
 * Author: Jennifer Buehler
 * Date: November 2016
 */
#include <collision_benchmark/GazeboPhysicsWorld.hh>
#include <collision_benchmark/GazeboWorldState.hh>
#include <collision_benchmark/GazeboStateCompare.hh>
#include <collision_benchmark/GazeboHelpers.hh>
#include <collision_benchmark/GazeboWorldLoader.hh>
#include <collision_benchmark/boost_std_conversion.hh>

#include <gazebo/physics/physics.hh>

using collision_benchmark::GazeboPhysicsWorld;
using collision_benchmark::Contact;
using collision_benchmark::ContactInfo;

GazeboPhysicsWorld::GazeboPhysicsWorld(bool _enforceContactComputation)
  : enforceContactComputation(_enforceContactComputation),
    paused(false)
{
}

GazeboPhysicsWorld::~GazeboPhysicsWorld()
{
}

bool GazeboPhysicsWorld::SupportsSDF() const
{
  return true;
}

bool GazeboPhysicsWorld::WaitForNamespace
      (const gazebo::physics::WorldPtr& gzworld, float maxWait, float waitSleep)
{
  std::string worldNamespace = gzworld->Name();

  // wait for namespace to be loaded, to make sure the order of
  // namespaces maintained in the transport system eventually will correspond
  // to the same order of the worlds
  if (!collision_benchmark::WaitForNamespace(worldNamespace,
                                             maxWait, waitSleep))
  {
    std::cerr << "Namespace of world '" << worldNamespace
              << "' was not loaded" << std::endl;
    return false;
  }
  return true;
}


collision_benchmark::OpResult
GazeboPhysicsWorld::LoadFromSDF(const sdf::ElementPtr& sdf,
                                const std::string& worldname)
{
  gazebo::physics::WorldPtr gzworld =
    collision_benchmark::LoadWorldFromSDF(sdf, worldname);

  if (!gzworld)
    return collision_benchmark::FAILED;

  if (OnLoadWaitForNamespace &&
      !WaitForNamespace(gzworld, OnLoadMaxWaitForNamespace,
                        OnLoadWaitForNamespaceSleep))
    return collision_benchmark::FAILED;

  SetWorld(collision_benchmark::to_std_ptr<gazebo::physics::World>(gzworld));
  return collision_benchmark::SUCCESS;
}

collision_benchmark::OpResult
GazeboPhysicsWorld::LoadFromFile(const std::string& filename,
                                 const std::string& worldname)
{
  gazebo::physics::WorldPtr gzworld =
    collision_benchmark::LoadWorldFromFile(filename, worldname);

  if (!gzworld)
    return collision_benchmark::FAILED;

  if (OnLoadWaitForNamespace &&
      !WaitForNamespace(gzworld, OnLoadMaxWaitForNamespace,
                        OnLoadWaitForNamespaceSleep))
    return collision_benchmark::FAILED;

  SetWorld(collision_benchmark::to_std_ptr<gazebo::physics::World>(gzworld));
  return collision_benchmark::SUCCESS;
}

collision_benchmark::OpResult
GazeboPhysicsWorld::LoadFromString(const std::string& str,
                                   const std::string& worldname)
{
  gazebo::physics::WorldPtr gzworld =
    collision_benchmark::LoadWorldFromSDFString(str, worldname);

  if (!gzworld)
    return collision_benchmark::FAILED;

  if (OnLoadWaitForNamespace &&
      !WaitForNamespace(gzworld, OnLoadMaxWaitForNamespace,
                        OnLoadWaitForNamespaceSleep))
    return collision_benchmark::FAILED;

  SetWorld(collision_benchmark::to_std_ptr<gazebo::physics::World>(gzworld));
  return collision_benchmark::SUCCESS;

}

GazeboPhysicsWorld::ModelLoadResult
GazeboPhysicsWorld::AddModelFromFile(const std::string& filename,
                                     const std::string& modelname)
{
  sdf::ElementPtr sdfRoot =
    collision_benchmark::GetSDFElementFromFile(filename, "model", modelname);
  ModelLoadResult ret;
  ret.opResult=FAILED;
  if (!sdfRoot)
  {
    std::cerr<< " Could not get SDF for model in "<<filename<<std::endl;
    return ret;
  }

  ret = AddModelFromSDF(sdfRoot);
  return ret;
}

GazeboPhysicsWorld::ModelLoadResult
GazeboPhysicsWorld::AddModelFromString(const std::string& str,
                                       const std::string& modelname)
{
  ModelLoadResult ret;
  ret.opResult=FAILED;

  std::string useStr=str;
  int checkSDF=collision_benchmark::isProperSDFString(useStr);
  if (checkSDF<0)
  {
    if (checkSDF==-2)
    {
      collision_benchmark::wrapSDF(useStr);
    }
    else
    {
      std::cerr<<"SDF is not proper so cannot load model from the string. "
               <<"Value: "<<checkSDF<<std::endl;
      return ret;
    }
  }

  sdf::ElementPtr sdfRoot =
    collision_benchmark::GetSDFElementFromString(useStr, "model", modelname);
  if (!sdfRoot)
  {
    std::cerr<< " Could not get SDF for model."<<std::endl;
    return ret;
  }

  ret = AddModelFromSDF(sdfRoot);
  return ret;

}

GazeboPhysicsWorld::ModelLoadResult
GazeboPhysicsWorld::AddModelFromSDF(const sdf::ElementPtr& sdf,\
                                    const std::string& modelname)
{
  gazebo::physics::ModelPtr model =
    collision_benchmark::LoadModelFromSDF(sdf, world, modelname);
  ModelLoadResult ret;
  if (!model)
  {
    ret.opResult=FAILED;
    return ret;
  }
  ret.opResult=SUCCESS;
  ret.modelID=model->GetName();
  return ret;
}

bool GazeboPhysicsWorld::SupportsShapes() const
{
  return true;
}

GazeboPhysicsWorld::ModelLoadResult
GazeboPhysicsWorld::AddModelFromShape(const std::string& modelname,
                                      const Shape::Ptr& shape,
                                      const Shape::Ptr collShape)
{
  ModelLoadResult ret;
  ret.opResult=FAILED;

  if (modelname.empty())
  {
    std::cerr<<"Must specify model name"<<std::endl;
    return ret;
  }

  // Build the SDF
  sdf::ElementPtr root(new sdf::Element());
  root->SetName("model");
  root->AddAttribute("name", "string", modelname, true, "model name");

  sdf::ElementPtr pose=shape->GetPoseSDF();
  root->InsertElement(pose);

  sdf::ElementPtr link(new sdf::Element());
  link->SetName("link");
  link->AddAttribute("name", "string", "link", true, "link name");
  root->InsertElement(link);

  sdf::ElementPtr shapeGeom=shape->GetShapeSDF(true,false);
  sdf::ElementPtr visual(new sdf::Element());
  visual->SetName("visual");
  visual->AddAttribute("name", "string", "visual", true, "visual name");
  visual->InsertElement(shapeGeom);
  link->InsertElement(visual);

  sdf::ElementPtr shapeColl=shapeGeom;
  if (shape->SupportLowRes())
    shapeColl = shape->GetShapeSDF(false,false);
  sdf::ElementPtr collision(new sdf::Element());
  collision->SetName("collision");
  collision->AddAttribute("name", "string", "collision",
                          true, "collision name");
  collision->InsertElement(shapeColl);
  link->InsertElement(collision);

  // std::cout<<"SDF shape: "<<root->ToString("");

  return AddModelFromSDF(root);
}

std::vector<GazeboPhysicsWorld::ModelID>
GazeboPhysicsWorld::GetAllModelIDs() const
{
  std::vector<GazeboPhysicsWorld::ModelID> names;
  int numModels = world->ModelCount();
  for (int i=0; i<numModels; ++i)
  {
    gazebo::physics::ModelPtr m=world->ModelByIndex(i);
    names.push_back(m->GetName());
  }
  return names;
}

bool GazeboPhysicsWorld::RemoveModel(const ModelID& id)
{
  gazebo::physics::ModelPtr m=world->ModelByName(id);
  if (!m) return false;
  world->RemoveModel(m);
  return true;
}

void GazeboPhysicsWorld::Clear()
{
  collision_benchmark::ClearModels(world);
}

GazeboPhysicsWorld::WorldState GazeboPhysicsWorld::GetWorldState() const
{
  gazebo::physics::WorldState s(world);
  return s;
}

GazeboPhysicsWorld::WorldState
GazeboPhysicsWorld::GetWorldStateDiff(const WorldState& other) const
{
  gazebo::physics::WorldState diffState = other - GetWorldState();
  // std::cout << "Diff state: " << std::endl << diffState << std::endl;
  return diffState;
}

collision_benchmark::OpResult
GazeboPhysicsWorld::SetWorldState(const WorldState& state, bool isDiff)
{
  collision_benchmark::SetWorldState(world, state);

#ifdef DEBUG
  gazebo::physics::WorldState _currentState(world);
  GazeboStateCompare::Tolerances t =
    GazeboStateCompare::Tolerances::CreateDefault(1e-03);
  if (!world->PhysicsEnabled()) t.CheckDynamics=false;
  if (!GazeboStateCompare::Equal(_currentState, state, t))
  {
    std::cerr<<"Target state was not set as supposed to!!"<<std::endl;
  }
#endif

  return collision_benchmark::SUCCESS;
}


bool GazeboPhysicsWorld::SetBasicModelState(const ModelID  &_id,
                                            const BasicState &_state)
{
  gazebo::physics::ModelPtr m=world->ModelByName(_id);
  if (!m)
  {
    std::cerr<<"Model " << _id << " could not be found" << std::endl;
    return false;
  }
  ignition::math::Pose3d pose = m->WorldPose();
  if (_state.PosEnabled()) pose.Pos().Set(_state.position.x,
                                          _state.position.y,
                                          _state.position.z);
  if (_state.RotEnabled()) pose.Rot().Set(_state.rotation.w,
                                          _state.rotation.x,
                                          _state.rotation.y,
                                          _state.rotation.z);

  // std::cout<<"Setting world state "<<_state<<std::endl;

  m->SetWorldPose(pose);

  if (_state.ScaleEnabled())
  {
    ignition::math::Vector3d s(_state.scale.x,
                               _state.scale.y,
                               _state.scale.z);
    m->SetScale(s);
  }
  return true;
}

bool GazeboPhysicsWorld::GetBasicModelState(const ModelID  &_id,
                                            BasicState &_state)
{
  gazebo::physics::ModelPtr m=world->ModelByName(_id);
  if (!m)
  {
    std::cerr<<"Model " << _id << " could not be found" << std::endl;
    return false;
  }
  ignition::math::Pose3d pose = m->WorldPose();
  _state.SetPosition(pose.Pos().X(), pose.Pos().Y(), pose.Pos().Z());
  _state.SetRotation(pose.Rot().X(), pose.Rot().Y(),
                     pose.Rot().Z(), pose.Rot().W());
  ignition::math::Vector3d scale = m->Scale();
  _state.SetScale(scale.X(), scale.Y(), scale.Z());
  return true;
}

#define NEW_WORLDRUN_SOLUTION

void GazeboPhysicsWorld::PostWorldLoaded()
{
  GZ_ASSERT(world, "World is null, must have been loaded!");
#ifdef NEW_WORLDRUN_SOLUTION
  // Stop the world just in case it has been started already.
  // We want to start it under controlled conitions here.
  world->Stop();
  world->SetPaused(true);
  // Run the world in paused mode so that the main
  // loop of the gazebo world is working and calls of
  // Update() will do a World::Step().
  world->Run(0);
#endif
}


void GazeboPhysicsWorld::Update(int steps, bool force)
{
  // std::cout<<"Running "<<steps<<" steps for world "
  //          <<world->Name()<<", physics engine: "
  //          <<world->Physics()->GetType()<<std::endl;
#ifdef NEW_WORLDRUN_SOLUTION
  if (!force && IsPaused()) return;

  // if the world is not paused, it is updating itself already
  // automatically (started in PostWorldLoaded().
  // We should either return and don't call
  // Step(), or first pause the world.
  if (!world->IsPaused())
  {
    static bool printOnce=true;
    if (printOnce)
    {
      std::cout<<"DEBUG WARNING: The Gazebo world is not paused. "
        <<"In GazeboPhysicsWorld::Update(), we operate it in "
        <<"paused mode and rely on manually doint the updates "
        <<"instead of letting the gazebo world update "
        <<"itself continuously."<<std::endl;
      printOnce=false;
    }
    world->SetPaused(true);
  }

  // Step() only works if the world is paused.
  // It advances the state despite the paused state.
  world->Step(steps);
#else
  // This method calls world->RunBlocking();
  gazebo::runWorld(world, steps);
  // iterations is always 1 if it has been set with steps!=0
  // in call above. Should fix this in Gazebo::World?
  // std::cout<<"Iterations: "<<world->Iterations()<<std::endl;
#endif
}

void GazeboPhysicsWorld::SetPaused(bool flag)
{
#ifdef NEW_WORLDRUN_SOLUTION
  paused = flag;
#else
  world->SetPaused(flag);
#endif
}

bool GazeboPhysicsWorld::IsPaused() const
{
#ifdef NEW_WORLDRUN_SOLUTION
  return paused;
#else
  return world->IsPaused();
#endif
}

std::string GazeboPhysicsWorld::GetName() const
{
  return world->Name();
}

bool GazeboPhysicsWorld::SupportsContacts() const
{
  return true;
}

// helper function which can be used to get contact info of either
// all models (m1 and m2 set to NULL), or for one model
// (m1=NULL and m2=NULL) or for two models (m1!=NULL and m2!=NULL).
std::vector<GazeboPhysicsWorld::ContactInfoPtr>
GetContactInfoHelper(const gazebo::physics::WorldPtr& world,
                     const GazeboPhysicsWorld::ModelID * m1=NULL,
                     const GazeboPhysicsWorld::ModelID * m2=NULL)
{
  std::vector<GazeboPhysicsWorld::ContactInfoPtr> ret;
  const gazebo::physics::ContactManager* contactManager =
    world->Physics()->GetContactManager();
  GZ_ASSERT(contactManager, "Contact manager has to be set");
  const std::vector<gazebo::physics::Contact*>& contacts =
    contactManager->GetContacts();
  // std::cout<<"World has "<<contacts.size()<<"contacts."<<std::endl;
  for (std::vector<gazebo::physics::Contact*>::const_iterator
       it =contacts.begin(); it!=contacts.end(); ++it)
  {
    const gazebo::physics::Contact * c = *it;
    GZ_ASSERT(c->collision1->GetModel(), "Model of collision1 must be set");
    GZ_ASSERT(c->collision1->GetLink(), "Link of collision1 must be set");
    GZ_ASSERT(c->collision2->GetModel(), "Model of collision2 must be set");
    GZ_ASSERT(c->collision2->GetLink(), "Link of collision2 must be set");

    std::string m1Name=c->collision1->GetModel()->GetName();
    std::string m2Name=c->collision2->GetModel()->GetName();

    if (m1)
    {
      if (m2)
      { // m1Name and m2Name do not correspond to m1 and m2
        if ((*m1!=m1Name || *m2!=m2Name) &&
            (*m1!=m2Name || *m2!=m1Name)) continue;
      }
      else
      { // m1 has to be m1Name or m2Name to continue
        if (*m1!=m1Name && *m1!=m2Name) continue;
      }
    }

    GazeboPhysicsWorld::ContactInfoPtr
      cInfo(new GazeboPhysicsWorld::ContactInfo
            (m1Name, c->collision1->GetLink()->GetName(),
             m2Name, c->collision2->GetLink()->GetName()));
    for (int i=0; i < c->count; ++i)
    {
      cInfo->contacts.push_back
        (GazeboPhysicsWorld::Contact(c->positions[i], c->normals[i],
                                     c->wrench[i], c->depths[i]));
    }

    if (cInfo->contacts.empty())
      std::cerr << "CONSISTENCY GazeboPhysicsWorld: If there are no contacts, "
                << "there should be no collision!! World: " << world->Name()
                << " Models: " << m1Name << ", " << m2Name << std::endl;
    else
      ret.push_back(cInfo);
  }
  return ret;
}

// deleter which does nothing, to be used for
// std::shared_ptr with extreme caution!
template<typename Type>
void null_deleter(Type *){}

// helper function which can be used to get contact info of either
// all models (m1 and m2 set to NULL), or for one model
// (m1=NULL and m2=NULL) or for two models (m1!=NULL and m2!=NULL).
std::vector<GazeboPhysicsWorld::NativeContactPtr>
GetNativeContactsHelper(const gazebo::physics::WorldPtr& world,
                        const GazeboPhysicsWorld::ModelID * m1=NULL,
                        const GazeboPhysicsWorld::ModelID * m2=NULL)
{
  std::vector<GazeboPhysicsWorld::NativeContactPtr> ret;

  const gazebo::physics::ContactManager* contactManager
    = world->Physics()->GetContactManager();
  GZ_ASSERT(contactManager, "Contact manager has to be set");
  const std::vector<gazebo::physics::Contact*>& contacts =
    contactManager->GetContacts();
  for (std::vector<gazebo::physics::Contact*>::const_iterator
       it=contacts.begin(); it!=contacts.end(); ++it)
  {
      gazebo::physics::Contact * c=*it;
      GZ_ASSERT(c->collision1->GetModel(), "Model of collision1 must be set");
      GZ_ASSERT(c->collision1->GetLink(), "Link of collision1 must be set");
      GZ_ASSERT(c->collision2->GetModel(), "Model of collision2 must be set");
      GZ_ASSERT(c->collision2->GetLink(), "Link of collision2 must be set");

      std::string m1Name=c->collision1->GetModel()->GetName();
      std::string m2Name=c->collision2->GetModel()->GetName();

      if (m1)
      {
        if (m2)
        { // m1Name and m2Name do not correspond to m1 and m2
          if ((*m1!=m1Name || *m2!=m2Name) &&
              (*m1!=m2Name || *m2!=m1Name)) continue;
        }
        else
        { // m1 has to be m1Name or m2Name to continue
          if (*m1!=m1Name && *m1!=m2Name) continue;
        }
      }

      // XXX HACK -> Also remove warning in header documentation of
      // GetNativeContacts() when this is resolved!
      // While Gazebo doesn't manage contacts as shared pointers, unfortunately
      // we will need to return the std::shared_ptr<gazebo::physics::Contact>
      // pointers without deleter. This may lead to awful segfaults if the
      // contacts are used beyond their lifetime in Gazebo.
      // However it is expected (?) that soon Gazebo will use shared pointers
      // for this as well, so keep this flakey solution for now.
      GazeboPhysicsWorld::NativeContactPtr
        gzContact(c, &null_deleter<GazeboPhysicsWorld::NativeContact>);
      ret.push_back(gzContact);
  }
  return ret;
}


std::vector<GazeboPhysicsWorld::ContactInfoPtr>
GazeboPhysicsWorld::GetContactInfo() const
{
  return GetContactInfoHelper(world);
}

std::vector<GazeboPhysicsWorld::ContactInfoPtr>
GazeboPhysicsWorld::GetContactInfo(const ModelID& m1, const ModelID& m2) const
{
  return GetContactInfoHelper(world, &m1, &m2);
}

std::vector<GazeboPhysicsWorld::NativeContactPtr>
GazeboPhysicsWorld::GetNativeContacts() const
{
  return GetNativeContactsHelper(world);
}

std::vector<GazeboPhysicsWorld::NativeContactPtr>
GazeboPhysicsWorld::GetNativeContacts(const ModelID& m1,
                                      const ModelID& m2) const
{
  return GetNativeContactsHelper(world, &m1, &m2);
}


bool GazeboPhysicsWorld::IsAdaptor() const
{
  return true;
}

#ifndef CONTACTS_ENFORCABLE
void GazeboPhysicsWorld::OnContact(ConstContactsPtr &_msg)
{
//  std::cout<<"DEBUG: Got contact!"<<std::endl;
}
#endif

void GazeboPhysicsWorld::SetEnforceContactsComputation(bool flag)
{
  enforceContactComputation=flag;
#ifndef CONTACTS_ENFORCABLE
  if (enforceContactComputation)
  {
    if (!node)
    {
      node = gazebo::transport::NodePtr(new gazebo::transport::Node());
      node->Init(GetName());
    }
    contactsSub = node->Subscribe("~/physics/contacts",
                                  &GazeboPhysicsWorld::OnContact, this);
  }
  else
  {
    contactsSub.reset();
  }
#else
  assert(world->Physics() && world->Physics()->GetContactManager());
  world->Physics()->GetContactManager()->SetEnforceContacts(flag);
#endif
}

collision_benchmark::RefResult
GazeboPhysicsWorld::SetWorld(const WorldPtr& _world)
{
  world = collision_benchmark::to_boost_ptr<World>(_world);
  SetEnforceContactsComputation(enforceContactComputation);
  PostWorldLoaded();
  return collision_benchmark::REFERENCED;
}

GazeboPhysicsWorld::WorldPtr GazeboPhysicsWorld::GetWorld() const
{
  return collision_benchmark::to_std_ptr<World>(world);
}

GazeboPhysicsWorld::ModelPtr
GazeboPhysicsWorld::GetModel(const ModelID& model) const
{
  gazebo::physics::ModelPtr m=world->ModelByName(model);
  return collision_benchmark::to_std_ptr<gazebo::physics::Model>(m);
}


bool GazeboPhysicsWorld::GetAABB(const ModelID& id,
                                 Vector3& min, Vector3& max) const
{
  gazebo::physics::ModelPtr m=world->ModelByName(id);
  if (!m) return false;
  ignition::math::Box box = m->BoundingBox();
  min = Vector3(box.Min().X(), box.Min().Y(), box.Min().Z());
  max = Vector3(box.Max().X(), box.Max().Y(), box.Max().Z());
  return true;
}

GazeboPhysicsWorld::PhysicsEnginePtr
GazeboPhysicsWorld::GetPhysicsEngine() const
{
  if (world)
    return collision_benchmark::to_std_ptr<GazeboPhysicsWorld::PhysicsEngine>
            (world->Physics());
  return PhysicsEnginePtr();
}

void GazeboPhysicsWorld::SetDynamicsEnabled(const bool flag)
{
  if (world) world->SetPhysicsEnabled(flag);
}

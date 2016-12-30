/*
 * Copyright (C) 2014-2016 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <sstream>
#include <gazebo/msgs/msgs.hh>
#include "ClientGui.hh"

using collision_benchmark::ClientGui;

// Register this plugin with the simulator
GZ_REGISTER_GUI_PLUGIN(ClientGui)


QSize maxHeightAddWidth(const QSize& s1, const QSize& s2, float wFact=1, float hFact=1.0)
{
  return QSize((s1.width() + s2.width())*wFact,
               std::max(s1.height(), s2.height())*hFact);
}



/////////////////////////////////////////////////
ClientGui::ClientGui()
  : GUIPlugin()
{
  // Set the frame background and foreground colors
  this->setStyleSheet("QFrame { background-color : rgba(100, 100, 100, 255); color : white; }");

  // Create the main layout
  QHBoxLayout *mainLayout = new QHBoxLayout;

  // Create the frame & layout to hold all the buttons
  QFrame *switchWorldsFrame = new QFrame();
  QHBoxLayout *switchWorldsLayout = new QHBoxLayout();

  // Create the push buttons and connect to OnButton* functions
  QPushButton * buttonPrev = new QPushButton("Prev");
  QPushButton * buttonNext = new QPushButton("Next");
  buttonPrev->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  buttonPrev->resize(buttonPrev->sizeHint());
  buttonNext->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  buttonNext->resize(buttonNext->sizeHint());

  connect(buttonPrev, SIGNAL(clicked()), this, SLOT(OnButtonPrev()));
  connect(buttonNext, SIGNAL(clicked()), this, SLOT(OnButtonNext()));

  minSize = maxHeightAddWidth(buttonPrev->sizeHint(), buttonNext->sizeHint());

  // Create label to sit in-between buttons ad display world name
  labelName = new QLabel("<...>");

  // Add the buttons to the frame's layout
  switchWorldsLayout->addWidget(buttonPrev);
  switchWorldsLayout->addWidget(labelName);
  switchWorldsLayout->addWidget(buttonNext);

  switchWorldsFrame->setLayout(switchWorldsLayout);
  mainLayout->addWidget(switchWorldsFrame);

  // Remove margins to reduce space
  switchWorldsLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  this->setLayout(mainLayout);

  // Position and resize this widget
  QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  this->setSizePolicy(sizePolicy);
  this->move(10, 10);
  QSize totalSize = maxHeightAddWidth(labelName->sizeHint(), minSize, 1.1);
  this->resize(totalSize);

  // Set up transportation system
  this->node = gazebo::transport::NodePtr(new gazebo::transport::Node());
  this->node->Init();
  this->mirrorWorldPub = this->node->Advertise<gazebo::msgs::Any>("mirror_world/set_world");
  this->mirrorWorldSub = this->node->Subscribe("mirror_world/get_world", &ClientGui::receiveWorldMsg, this);
}

/////////////////////////////////////////////////
ClientGui::~ClientGui()
{
}

void ClientGui::receiveWorldMsg(ConstAnyPtr &_msg)
{
  std::cout << "Any: "<<_msg->DebugString();
  std::string worldName=_msg->string_value();
  labelName->setText(worldName.c_str());
  QSize totalSize = maxHeightAddWidth(labelName->sizeHint(), minSize, 1.1);
  this->resize(totalSize);
}


/////////////////////////////////////////////////
void ClientGui::OnButtonNext()
{
  // Send the model to the gazebo server
  gazebo::msgs::Any m;
  m.set_type(gazebo::msgs::Any::INT32);
  m.set_int_value(1); // "Next" world
  this->mirrorWorldPub->Publish(m);
}


/////////////////////////////////////////////////
void ClientGui::OnButtonPrev()
{
  // Send the model to the gazebo server
  gazebo::msgs::Any m;
  m.set_type(gazebo::msgs::Any::INT32);
  m.set_int_value(-1); // "Prev" world
  this->mirrorWorldPub->Publish(m);
}

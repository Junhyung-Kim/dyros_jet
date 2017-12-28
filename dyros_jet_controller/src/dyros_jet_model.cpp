#include "dyros_jet_controller/dyros_jet_model.h"



namespace dyros_jet_controller
{

// https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char
constexpr const char* DyrosJetModel::EE_NAME[4];
constexpr const size_t DyrosJetModel::HW_TOTAL_DOF;
constexpr size_t DyrosJetModel::MODEL_WITH_VJOINT_DOF;
constexpr const size_t DyrosJetModel::MODEL_DOF;


// These should be replaced by YAML or URDF or something
const std::string DyrosJetModel::JOINT_NAME[DyrosJetModel::HW_TOTAL_DOF] = {
  "L_HipYaw","L_HipRoll","L_HipPitch","L_KneePitch","L_AnklePitch","L_AnkleRoll",
  "R_HipYaw","R_HipRoll","R_HipPitch","R_KneePitch","R_AnklePitch","R_AnkleRoll",
  "WaistPitch","WaistYaw",
  "L_ShoulderPitch","L_ShoulderRoll","L_ShoulderYaw","L_ElbowRoll","L_WristYaw","L_WristRoll","L_HandYaw",
  "R_ShoulderPitch","R_ShoulderRoll","R_ShoulderYaw","R_ElbowRoll","R_WristYaw","R_WristRoll","R_HandYaw",
  "HeadYaw", "HeadPitch", "R_Gripper", "L_Gripper"};

// TODO: How to get joint map directly from RBDL?
/*
const std::map<string, unsigned int> JOINT_MAP = {
  {"L_HipYaw", 0},
  {"L_HipRoll", 1},"L_HipPitch","L_KneePitch","L_AnklePitch","L_AnkleRoll",
  "R_HipYaw","R_HipRoll","R_HipPitch","R_KneePitch","R_AnklePitch","R_AnkleRoll",
  "WaistPitch","WaistYaw",
  "L_ShoulderPitch","L_ShoulderRoll","L_ShoulderYaw","L_ElbowRoll","L_WristYaw","L_WristRoll","L_HandYaw",
  "R_ShoulderPitch","R_ShoulderRoll","R_ShoulderYaw","R_ElbowRoll","R_WristYaw","R_WristRoll","R_HandYaw",
  "HeadYaw", "HeadPitch", "R_Gripper", "L_Gripper"
};
*/
// Dynamixel Hardware ID
const int DyrosJetModel::JOINT_ID[DyrosJetModel::HW_TOTAL_DOF] = {
  16,18,20,22,24,26,  // 6
  15,17,19,21,23,25,  // 6
  28,27, // waist yaw - roll order  2
  2,4,6,8,10,12,14,   // 7
  1,3,5,7,9,11,13,    // 7
  29,30,31,32};       // 4

DyrosJetModel::DyrosJetModel() :
  joint_start_index_{6, 12, 20, 27} // Virtual Joint + 6
{
  A_temp_.resize(MODEL_WITH_VJOINT_DOF, MODEL_WITH_VJOINT_DOF);
  base_position_.setZero();
  q_.setZero();

  std::string desc_package_path = ros::package::getPath("dyros_jet_description");
  std::string urdf_path = desc_package_path + "/robots/dyros_jet_robot.urdf";

  ROS_INFO("Loading DYROS JET description from = %s",urdf_path.c_str());
  RigidBodyDynamics::Addons::URDFReadFromFile(urdf_path.c_str(), &model_, true,true);
  ROS_INFO("Successfully loaded.");
  ROS_INFO("Total DoF = %d", model_.dof_count);
  ROS_INFO("Total DoF = %d", model_.q_size);
  //model_.mJoints[0].)
  if(model_.dof_count != MODEL_WITH_VJOINT_DOF)
  {
    ROS_WARN("The DoF in the model file and the code do not match.");
    ROS_WARN("Model file = %d, Code = %d", model_.dof_count, (int)MODEL_WITH_VJOINT_DOF);
  }

  // waist = 12
  //joint_start_index_[EE_LEFT_HAND] = 14;
  //joint_start_index_[EE_RIGHT_HAND] = 21;

  for (size_t i=0; i<4; i++)
  {
    end_effector_id_[i] = model_.GetBodyId(EE_NAME[i]);
    ROS_INFO("%s: id - %d",EE_NAME[i], end_effector_id_[i]);
    std::cout << model_.mBodies[end_effector_id_[i]].mCenterOfMass << std::endl;
  }
}


void DyrosJetModel::test()
{



  updateKinematics(Eigen::Vector28d::Zero());

  I_.setIdentity();
  j_c_ = leg_with_vlink_jacobian_[0];
  lambda_c_ = (j_c_*leg_A_.inverse()*j_c_.transpose()).inverse();
  p_c_ = j_c_.transpose()*lambda_c_*j_c_*leg_A_.inverse();
  std::cout << "leg_jacobian_" << std::endl;
  /*
  std::cout << leg_jacobian_[0] << std::endl << std::endl;
  std::cout << leg_jacobian_[1] << std::endl;
  std::cout << "arm_jacobian_" << std::endl;
  std::cout << arm_jacobian_[0] << std::endl << std::endl;
  std::cout << arm_jacobian_[1] << std::endl;
  std::cout << "currnet_transform_" << std::endl;
  std::cout << currnet_transform_[0].translation() << std::endl << std::endl;
  std::cout << currnet_transform_[1].translation() << std::endl << std::endl;
  std::cout << currnet_transform_[2].translation() << std::endl << std::endl;
  std::cout << currnet_transform_[3].translation() << std::endl << std::endl;
  std::cout << "full_inertia_" << std::endl;
  std::cout << A_<< std::endl << std::endl << std::endl;
  */
  std::cout << "p_c_" << std::endl;

  std::cout << p_c_ << std::endl << std::endl << std::endl;

  std::cout << "lambda_c_" << std::endl;
  std::cout << lambda_c_<< std::endl << std::endl << std::endl;

  //std::cout << leg_A_.inverse()-leg_A_.inverse()*(I_ - p_c_)<< std::endl << std::endl << std::endl;
}

void DyrosJetModel::updateKinematics(const Eigen::VectorXd& q)
{
  RigidBodyDynamics::UpdateKinematicsCustom(model_, &q, NULL, NULL);
  q_.segment<28>(6) = q;
  for(unsigned int i=0; i<4; i++)
  {
    getTransformEndEffector((EndEffector)i, &currnet_transform_[i]);
    if (i < 2)
    {
      getJacobianMatrix6DoF((EndEffector)i, &leg_jacobian_[i]);
      getJacobianMatrix18DoF((EndEffector)i, &leg_with_vlink_jacobian_[i]);
    }
    else
    {
      getJacobianMatrix7DoF((EndEffector)i, &arm_jacobian_[i-2]);
    }
  }
  getInertiaMatrix34DoF(&A_);
  getInertiaMatrix18DoF(&leg_A_);



}

void DyrosJetModel::getTransformEndEffector // must call updateKinematics before calling this function
(EndEffector ee, Eigen::Isometry3d* transform_matrix)
{
  Eigen::Vector3d gghg = RigidBodyDynamics::CalcBodyToBaseCoordinates(model_, q_,end_effector_id_[ee], base_position_, false);
  transform_matrix->translation() = RigidBodyDynamics::CalcBodyToBaseCoordinates
      (model_, q_,end_effector_id_[ee], base_position_, false);
  transform_matrix->linear() = RigidBodyDynamics::CalcBodyWorldOrientation(
        model_, q_, end_effector_id_[ee], false);
}

void DyrosJetModel::getTransformEndEffector // must call updateKinematics before calling this function
(EndEffector ee, Eigen::Vector3d* position, Eigen::Matrix3d* rotation)
{
  *position = RigidBodyDynamics::CalcBaseToBodyCoordinates
      (model_, q_,end_effector_id_[ee], base_position_, false);
  *rotation = RigidBodyDynamics::CalcBodyWorldOrientation(
        model_, q_, end_effector_id_[ee], false);
}

void DyrosJetModel::getTransformEndEffector
(EndEffector ee, const Eigen::VectorXd& q, bool update_kinematics,
 Eigen::Vector3d* position, Eigen::Matrix3d* rotation)
{
  Eigen::Matrix<double, MODEL_WITH_VJOINT_DOF, 1> q_new;
  q_new = q_;
  switch (ee)
  {
  case EE_LEFT_FOOT:
  case EE_RIGHT_FOOT:
    q_new.segment<6>(joint_start_index_[ee]) = q;
    break;
  case EE_LEFT_HAND:
  case EE_RIGHT_HAND:
    q_new.segment<7>(joint_start_index_[ee]) = q;
    break;
  }
  if (update_kinematics)
  {
    q_ = q_new;
  }
  *position = RigidBodyDynamics::CalcBaseToBodyCoordinates
      (model_,q_new,end_effector_id_[ee], base_position_, update_kinematics);
  *rotation = RigidBodyDynamics::CalcBodyWorldOrientation(
        model_, q_new, end_effector_id_[ee], update_kinematics);
  // RigidBodyDynamics::Calcpo
  // model_.mBodies[0].mCenterOfMass
}

void DyrosJetModel::getInertiaMatrix34DoF(Eigen::Matrix<double, 34, 34> *inertia)
{
  // Non-realtime
  Eigen::MatrixXd full_inertia(MODEL_WITH_VJOINT_DOF, MODEL_WITH_VJOINT_DOF);
  full_inertia.setZero();
  RigidBodyDynamics::CompositeRigidBodyAlgorithm(model_, q_, full_inertia, false);

  inertia->block<34, 34>(0, 0) = full_inertia.block<34, 34>(0, 0);
}

void DyrosJetModel::getInertiaMatrix18DoF(Eigen::Matrix<double, 18, 18> *leg_inertia)
{
  // Non-realtime
  Eigen::MatrixXd full_inertia(MODEL_WITH_VJOINT_DOF, MODEL_WITH_VJOINT_DOF);
  full_inertia.setZero();
  RigidBodyDynamics::CompositeRigidBodyAlgorithm(model_, q_, full_inertia, false);

  leg_inertia->block<18, 18>(0, 0) = full_inertia.block<18, 18>(0, 0);
}

void DyrosJetModel::getJacobianMatrix18DoF(EndEffector ee, Eigen::Matrix<double, 6, 18> *jacobian)
{
  // Non-realtime
  Eigen::MatrixXd full_jacobian(6,MODEL_WITH_VJOINT_DOF);
  full_jacobian.setZero();
  RigidBodyDynamics::CalcPointJacobian6D(model_, q_, end_effector_id_[ee],
                                         Eigen::Vector3d::Zero(), full_jacobian, false);
  jacobian->setZero();

  switch (ee)
  {
  case EE_LEFT_FOOT:
    // swap
    // Virtual Link
    jacobian->block<3, 6>(0, 0) = full_jacobian.block<3, 6>(3, 0);
    jacobian->block<3, 6>(3, 0) = full_jacobian.block<3, 6>(0, 0);

    // left Leg Link
    jacobian->block<3, 6>(0, 6) = full_jacobian.block<3, 6>(3, joint_start_index_[ee]);
    jacobian->block<3, 6>(3, 6) = full_jacobian.block<3, 6>(0, joint_start_index_[ee]);
    break;
  case EE_RIGHT_FOOT:
    // swap
    // Virtual Link
    jacobian->block<3, 6>(0, 0) = full_jacobian.block<3, 6>(3, 0);
    jacobian->block<3, 6>(3, 0) = full_jacobian.block<3, 6>(0, 0);

    // right Leg Link
    jacobian->block<3, 6>(0, 12) = full_jacobian.block<3, 6>(3, joint_start_index_[ee]);
    jacobian->block<3, 6>(3, 12) = full_jacobian.block<3, 6>(0, joint_start_index_[ee]);
    break;
  case EE_LEFT_HAND:
  case EE_RIGHT_HAND:
    //*jacobian = full_jacobian.block<6, 7>(0, joint_start_index_[ee]);
    ROS_ERROR("Arm is 7 DoF. Please call getJacobianMatrix7DoF");
    break;
  }
}

void DyrosJetModel::getJacobianMatrix6DoF(EndEffector ee, Eigen::Matrix<double, 6, 6> *jacobian)
{
  // Non-realtime
  Eigen::MatrixXd full_jacobian(6,MODEL_WITH_VJOINT_DOF);
  full_jacobian.setZero();
  RigidBodyDynamics::CalcPointJacobian6D(model_, q_, end_effector_id_[ee],
                                         Eigen::Vector3d::Zero(), full_jacobian, false);

  switch (ee)
  {
  case EE_LEFT_FOOT:
  case EE_RIGHT_FOOT:
    // swap
    jacobian->block<3, 6>(0, 0) = full_jacobian.block<3, 6>(3, joint_start_index_[ee]);
    jacobian->block<3, 6>(3, 0) = full_jacobian.block<3, 6>(0, joint_start_index_[ee]);
    break;
  case EE_LEFT_HAND:
  case EE_RIGHT_HAND:
    //*jacobian = full_jacobian.block<6, 7>(0, joint_start_index_[ee]);
    ROS_ERROR("Arm is 7 DoF. Please call getJacobianMatrix7DoF");
    break;
  }
}
void DyrosJetModel::getJacobianMatrix7DoF(EndEffector ee, Eigen::Matrix<double, 6, 7> *jacobian)
{
  // Non-realtime
  Eigen::MatrixXd full_jacobian(6,MODEL_WITH_VJOINT_DOF);
  full_jacobian.setZero();
  RigidBodyDynamics::CalcPointJacobian6D(model_, q_, end_effector_id_[ee],
                                         Eigen::Vector3d::Zero(), full_jacobian, false);

  switch (ee)
  {
  case EE_LEFT_FOOT:
  case EE_RIGHT_FOOT:
    // swap
    ROS_ERROR("Leg is 6 DoF. Please call getJacobianMatrix7DoF");
    break;
  case EE_LEFT_HAND:
  case EE_RIGHT_HAND:
    //*jacobian = full_jacobian.block<6, 7>(0, joint_start_index_[ee]);
    jacobian->block<3, 7>(0, 0) = full_jacobian.block<3, 7>(3, joint_start_index_[ee]);
    jacobian->block<3, 7>(3, 0) = full_jacobian.block<3, 7>(0, joint_start_index_[ee]);
    break;
  }
}

}

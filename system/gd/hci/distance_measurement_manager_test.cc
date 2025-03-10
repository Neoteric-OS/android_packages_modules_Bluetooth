/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hci/distance_measurement_manager.h"

#include <bluetooth/log.h>
#include <flag_macros.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hal/ranging_hal.h"
#include "hal/ranging_hal_mock.h"
#include "hci/acl_manager_mock.h"
#include "hci/controller.h"
#include "hci/controller_mock.h"
#include "hci/hci_layer.h"
#include "hci/hci_layer_fake.h"
#include "module.h"

using testing::Return;

namespace bluetooth {
namespace hci {
namespace {
class TestController : public testing::MockController {
protected:
  void Start() override {}
  void Stop() override {}
  void ListDependencies(ModuleList* /* list */) const override {}
};

class TestAclManager : public testing::MockAclManager {
protected:
  void Start() override {}
  void Stop() override {}
  void ListDependencies(ModuleList* /* list */) const override {}
};

class DistanceMeasurementManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_hci_layer_ = new HciLayerFake;                    // Ownership is transferred to registry
    mock_controller_ = new TestController;                 // Ownership is transferred to registry
    mock_ranging_hal_ = new hal::testing::MockRangingHal;  // Ownership is transferred to registry
    mock_acl_manager_ = new TestAclManager;                // Ownership is transferred to registry
    fake_registry_.InjectTestModule(&hal::RangingHal::Factory, mock_ranging_hal_);
    fake_registry_.InjectTestModule(&Controller::Factory, mock_controller_);
    fake_registry_.InjectTestModule(&HciLayer::Factory, test_hci_layer_);
    fake_registry_.InjectTestModule(&AclManager::Factory, mock_acl_manager_);

    client_handler_ = fake_registry_.GetTestModuleHandler(&HciLayer::Factory);
    ASSERT_NE(client_handler_, nullptr);

    EXPECT_CALL(*mock_controller_, SupportsBleChannelSounding()).WillOnce(Return(true));
    EXPECT_CALL(*mock_ranging_hal_, IsBound()).WillOnce(Return(true));

    handler_ = fake_registry_.GetTestHandler();
    dm_manager_ = fake_registry_.Start<DistanceMeasurementManager>(&thread_, handler_);
  }

  void TearDown() override {
    fake_registry_.SynchronizeModuleHandler(&DistanceMeasurementManager::Factory,
                                            std::chrono::milliseconds(20));
    fake_registry_.StopAll();
  }

protected:
  TestModuleRegistry fake_registry_;
  HciLayerFake* test_hci_layer_ = nullptr;
  TestController* mock_controller_ = nullptr;
  TestAclManager* mock_acl_manager_ = nullptr;
  hal::testing::MockRangingHal* mock_ranging_hal_ = nullptr;
  os::Thread& thread_ = fake_registry_.GetTestThread();
  os::Handler* client_handler_ = nullptr;
  os::Handler* handler_ = nullptr;

  DistanceMeasurementManager* dm_manager_ = nullptr;
};

TEST_F(DistanceMeasurementManagerTest, setup_teardown) {
  EXPECT_NE(mock_ranging_hal_->GetRangingHalCallback(), nullptr);
}

}  // namespace
}  // namespace hci
}  // namespace bluetooth

// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <memory>

#include "../../source/FeatureRegistry.h"
#include "../../source/config/Config.h"
#include "gtest/gtest.h"

using namespace std;
using namespace Aws::Iot::DeviceClient;
using namespace Aws::Iot::DeviceClient::Util;

class FakeFeature : public Feature
{
  public:
    FakeFeature(const std::string &name) : name(name) {}
    int start() override
    {
        started = true;
        stopped = false;
        return 0;
    }
    int stop() override
    {
        stopped = true;
        started = false;
        return 0;
    }
    std::string getName() { return name; }
    bool isStarted() { return started; }
    bool isStopped() { return stopped; }

  private:
    std::string name;
    bool started;
    bool stopped;
};

class TestFeatureRegistry : public ::testing::Test
{
  public:
    shared_ptr<FeatureRegistry> features;
    shared_ptr<FakeFeature> feature1;
    shared_ptr<FakeFeature> feature2;
    shared_ptr<FakeFeature> feature3;

    void SetUp() override
    {
        features = make_shared<FeatureRegistry>();
        feature1 = make_shared<FakeFeature>("feature-1");
        feature2 = make_shared<FakeFeature>("feature-2");
        feature3 = make_shared<FakeFeature>("feature-3");
    }
};

TEST_F(TestFeatureRegistry, AddFeaturesToRegistry)
{
    /**
     * Simple test to ensure features are added to the registry and the size if properly updated
     */
    features->add(feature1->getName(), feature1);
    ASSERT_EQ(1, features->getSize());

    features->add(feature2->getName(), feature2);
    ASSERT_EQ(2, features->getSize());

    features->add(feature3->getName(), feature3);
    ASSERT_EQ(3, features->getSize());
}

TEST_F(TestFeatureRegistry, AddFeatureDoesNotDuplicate)
{
    /**
     * Tests to ensure the idempotency of the add method
     */
    features->add(feature1->getName(), feature1);
    ASSERT_EQ(1, features->getSize());

    features->add(feature1->getName(), feature1);
    ASSERT_EQ(1, features->getSize());
}

TEST_F(TestFeatureRegistry, GetNonExistentFeature)
{
    /**
     * Tests that attempting to get a feature that does not exist in the registry will return nullptr
     */
    shared_ptr<Feature> feature = features->get("feature-1");
    ASSERT_EQ(nullptr, feature);
}

TEST_F(TestFeatureRegistry, GetFeatureByName)
{
    /**
     * Tests to that the get method properly returns what is being requested
     */
    features->add(feature1->getName(), feature1);
    shared_ptr<Feature> feature = features->get(feature1.get()->getName());
    ASSERT_EQ(feature1, feature);
}

TEST_F(TestFeatureRegistry, TestDisableFeature)
{
    /**
     * Tests that disable() properly disables a feature by setting the value in the key value pair to a nullptr
     */
    features->add(feature1->getName(), feature1);
    features->add(feature2->getName(), feature2);
    ASSERT_EQ(2, features->getSize());
    features->disable(feature1->getName());
    ASSERT_EQ(nullptr, features->get(feature1->getName()));
    features->disable(feature2->getName());
    ASSERT_EQ(nullptr, features->get(feature2->getName()));
}

TEST_F(TestFeatureRegistry, StartAllFeatures)
{
    /**
     * Simple test to check that start all evokes the start method of all the features in the registry
     */
    features->add(feature1->getName(), feature1);
    features->add(feature2->getName(), feature2);
    features->add(feature3->getName(), feature3);

    features->startAll();

    ASSERT_TRUE(feature1->isStarted());
    ASSERT_TRUE(feature2->isStarted());
    ASSERT_TRUE(feature3->isStarted());
    ASSERT_FALSE(feature1->isStopped());
    ASSERT_FALSE(feature2->isStopped());
    ASSERT_FALSE(feature3->isStopped());
}

TEST_F(TestFeatureRegistry, StopAllFeatures)
{
    /**
     * Simple test to check that start all evokes the stop method of all the features in the registry
     * Also checks that the feature pointers are removed from the registry
     */
    features->add(feature1->getName(), feature1);
    features->add(feature2->getName(), feature2);
    features->add(feature3->getName(), feature3);

    features->stopAll();
    ASSERT_TRUE(feature1->isStopped());
    ASSERT_TRUE(feature2->isStopped());
    ASSERT_TRUE(feature3->isStopped());
    ASSERT_FALSE(feature1->isStarted());
    ASSERT_FALSE(feature2->isStarted());
    ASSERT_FALSE(feature3->isStarted());

    ASSERT_EQ(nullptr, features->get(feature1->getName()));
    ASSERT_EQ(nullptr, features->get(feature2->getName()));
    ASSERT_EQ(nullptr, features->get(feature3->getName()));
}
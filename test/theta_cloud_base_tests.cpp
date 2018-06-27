#include "catch2/catch.hpp"
#include "ThetaCloud.h"
#include "SensorHandlerToken.h"
#include "Wire.h"

struct ThetaCloudFixture {
protected:
	ThetaCloud testedThetaCloud;
};

struct ThetaCloudWithExampleReadHandler : public ThetaCloudFixture {
	ThetaCloudWithExampleReadHandler()
	{
		readHandlerToken = testedThetaCloud.addReadHandler(
			[this](const ThetaCloud::Emit& emit) {
				emit(SensorData{std::string("tick"), std::string("value")});
		});
	}

	SensorHandlerTokenPtr readHandlerToken;
};

struct ThetaCloudWithExampleWriteHandler : public ThetaCloudFixture {
	const std::string TEST_TOPIC = "TestTopic";
	std::list<SensorData> writtenSensorData;
	ThetaCloudWithExampleWriteHandler()
	{
		testedThetaCloud.whenDataAvailable([this](const SensorData& data) {
			writtenSensorData.push_back(data);
		});
		writeHandlerToken = testedThetaCloud.addWriteHandler(TEST_TOPIC,
			[](const SensorData& data, const ThetaCloud::Emit& emit) {
				emit(data);
		});
	}

	SensorHandlerTokenPtr writeHandlerToken;
};

TEST_CASE_METHOD(ThetaCloudWithExampleReadHandler, "ReadCallbacksNotCalledWhenNotInitialized", "[thetaBase]") {
	testedThetaCloud.whenDataAvailable([&](const SensorData& data)
		{
			FAIL();
		});
	testedThetaCloud.tick();
}

TEST_CASE_METHOD(ThetaCloudWithExampleReadHandler, "ReadCallbacksCalledWhenInitialized", "[thetaBase]") {
	int callbackCounter = 0;
	testedThetaCloud.whenDataAvailable([&](const SensorData& data)
		{
			++callbackCounter;
			CHECK(data.name == std::string("tick"));
			CHECK(data.value == std::string("value"));
		});
	testedThetaCloud.init();
	testedThetaCloud.tick();
	CHECK(1 == callbackCounter);
	REQUIRE(Wire.history.size() == 1);
	CHECK(std::string("begin(2,14);") == Wire.history.front());
}

TEST_CASE_METHOD(ThetaCloudFixture, "NothingHappensWhenThereAreNoReadCallbacks", "[thetaBase]") {
	testedThetaCloud.whenDataAvailable([&](const SensorData& data)
		{
			FAIL();
		});
	testedThetaCloud.init();
	testedThetaCloud.tick();
}

TEST_CASE_METHOD(ThetaCloudFixture, "ServicesCanUnsubscribeByDeletingToken", "[thetaBase]") {
	int callbackCounter = 0;
	testedThetaCloud.whenDataAvailable([&](const SensorData& data)
		{
			++callbackCounter;
		});
	testedThetaCloud.init();
	auto token1 = testedThetaCloud.addReadHandler(
		[this](const ThetaCloud::Emit& emit) {
			emit(SensorData{std::string(), std::string()});
	});
	testedThetaCloud.tick();
	CHECK(1 == callbackCounter);
	callbackCounter = 0;
	{
		auto token2 = testedThetaCloud.addReadHandler(
			[this](const ThetaCloud::Emit& emit) {
				emit(SensorData{std::string(), std::string()});
		});
		testedThetaCloud.tick();
		CHECK(2 == callbackCounter);
	}
	callbackCounter = 0;
	testedThetaCloud.tick();
	CHECK(1 == callbackCounter);
}

TEST_CASE_METHOD(ThetaCloudWithExampleWriteHandler, "WritingCausesAnAction", "[thetaBase]") {
	const std::string EXAMPLE_WRITTEN_VALUE("example value");
	int callbackCounter = 0;
	testedThetaCloud.init();
	testedThetaCloud.write(SensorData{TEST_TOPIC, EXAMPLE_WRITTEN_VALUE});
	REQUIRE(writtenSensorData.size() == 1);
	SensorData receivedData = writtenSensorData.front();
	CHECK(EXAMPLE_WRITTEN_VALUE == receivedData.value);
	CHECK(TEST_TOPIC == receivedData.name);
}
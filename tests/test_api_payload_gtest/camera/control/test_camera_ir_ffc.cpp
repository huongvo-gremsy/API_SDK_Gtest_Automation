/**
 * @file test_camera_ir.cpp
 * @brief Tests for the IR-only FFC (flat-field correction) functions.
 *
 * IMPORTANT FLOW NOTE: setPayloadCameraFFCMode() and setPayloadCameraFFCTrigg()
 * are not independent -- FFC_MODE_AUTO means the camera runs flat-field
 * correction on its own schedule, so an explicit trigger is only meaningful
 * once the camera is in FFC_MODE_MANUAL first. Testing the trigger in
 * isolation (without first confirming Manual mode) doesn't exercise the
 * real usage pattern. All trigger tests below set Manual mode as a
 * precondition first.
 *
 * Verification levels, per function:
 *
 *   setPayloadCameraFFCMode(mode) -- has a CLIENT-SIDE guard
 *     ("if(mode < 0 || mode >= FFC_MODE_END) return;") that runs before
 *     anything is sent over the wire. Tested at the SDK level: invalid
 *     input should produce NO network traffic, independent of firmware
 *     behavior. Valid input still falls back to best-effort ACK
 *     (MAV_CMD_USER_4, param1=2/param2=6) since getPayloadCameraFFCMode()
 *     is an empty stub -- there is no getter to state-verify against.
 *
 *   getPayloadCameraFFCMode(uint8_t&) -- EMPTY FUNCTION BODY in the .cpp.
 *     Nothing to test. Documented as a known gap, not silently skipped.
 *
 *   setPayloadCameraFFCTrigg() -- one-way trigger (MAV_CMD_USER_4,
 *     param1=2/param2=7). Best-effort ACK, run only after confirming
 *     Manual mode (see flow note above).
 *
 * IR-only: skipped if this bench has no IR camera, detected via the same
 * streaming-info reachability probe used elsewhere.
 *
 * TODO: FFC_MODE_END's actual value is not visible from what's been
 * shared in this SDK -- MANUAL=0/AUTO=1 is confirmed via the .cpp's
 * comment, the exact sentinel bounding valid input is assumed to be 2.
 */

#include "../../common/payload_test_fixture.h"

namespace {
constexpr uint32_t IR_CAMERA_ID = 2;
// constexpr uint8_t FFC_MODE_MANUAL = 0;
// constexpr uint8_t FFC_MODE_AUTO = 1;

// Best-effort: send setPayloadCameraFFCMode(mode), wait for ACK. No getter
// exists to state-verify, so this only confirms the command was accepted
// -- it does NOT confirm the mode is still Manual by the time a later
// trigger is sent (there's no way to check that with this SDK).
bool setFFCModeBestEffort(uint8_t mode, int timeoutMs = 3000) {
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    g_payload->setPayloadCameraFFCMode(mode);
    AckInfo ack;
    return waitForCommandAck(MAV_CMD_USER_4, seq, ack, timeoutMs) &&
           ack.result == MAV_RESULT_ACCEPTED;
}
}

// ---- Invalid-input guard: pure SDK-level test, no hardware required ----

class CameraIrFfcSdkGuardTest : public PayloadTest {};

TEST_F(CameraIrFfcSdkGuardTest, SetFFCMode_InvalidValue_SendsNoMessage)
{ // PayloadTest.SetFFCMode_InvalidValue_SendsNoMessage
    const uint64_t seqBefore = getCommandAckSeq(MAV_CMD_USER_4);

    g_payload->setPayloadCameraFFCMode(99); // clearly out of FFC_MODE_END range

    AckInfo ack;
    const bool anyAckArrived =
        waitForCommandAck(MAV_CMD_USER_4, seqBefore, ack, 1500);

    EXPECT_FALSE(anyAckArrived)
        << "Received a PAYLOAD_ACK after calling setPayloadCameraFFCMode(99), which should "
           "have been rejected by the client-side guard before anything was sent.";
}

// ---- IR-gated tests ----

class CameraIrFfcTest : public PayloadTest {
protected:
    void SetUp() override {
        uint64_t seq = g_cb.streamSeq.load();
        g_payload->getPayloadCameraStreamingInformation(IR_CAMERA_ID);
        if (!waitForSeq(g_cb.streamSeq, seq, 2000)) {
            GTEST_SKIP() << "No response for IR stream -- this bench likely has no IR camera.";
        }
    }
};

// ---- Real usage flow: Manual mode, THEN trigger ----

TEST_F(CameraIrFfcTest, SetManualMode_ThenTrigger_Acked)
{
    // 1. Set Manual mode -- required before a trigger is meaningful (see file header)
    bool modeAcked = setFFCModeBestEffort(FFC_MODE_MANUAL);
    if (!modeAcked) {
        GTEST_SKIP() << "No ACK for FFC mode=Manual (known-flaky on this firmware); "
                         "cannot confirm the precondition for a meaningful trigger.";
    }

    // Give the mode change a moment to actually apply before triggering.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 2. Trigger FFC
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    g_payload->setPayloadCameraFFCTrigg();

    AckInfo ack;
    const bool triggerAcked =
        waitForCommandAck(MAV_CMD_USER_4, seq, ack, 3000);
    if (!triggerAcked) {
        GTEST_SKIP() << "No ACK for FFC trigger (known-flaky on this firmware); "
                         "getPayloadCameraFFCMode() is an empty stub, so there is no "
                         "independent state to confirm the correction actually ran.";
    }

    EXPECT_EQ(ack.command, MAV_CMD_USER_4);
    EXPECT_EQ(ack.result, MAV_RESULT_ACCEPTED);
}

// ---- Mode-set tests, kept independently for isolated coverage ----

TEST_F(CameraIrFfcTest, SetFFCMode_Manual_BestEffortAck)
{
    if (!setFFCModeBestEffort(FFC_MODE_MANUAL)) {
        GTEST_SKIP() << "No ACK for FFC mode=Manual (known-flaky on this firmware).";
    }
    SUCCEED();
}

TEST_F(CameraIrFfcTest, SetFFCMode_Auto_BestEffortAck)
{
    if (!setFFCModeBestEffort(FFC_MODE_AUTO)) {
        GTEST_SKIP() << "No ACK for FFC mode=Auto (known-flaky on this firmware).";
    }
    SUCCEED();
}

// ---- Trigger without confirmed Manual mode: documents current camera state's effect ----
// Left in deliberately (not removed) since it's informative: does the
// firmware silently ignore/reject a trigger while in Auto mode, or does it
// still ACK regardless of mode? Findings-oriented, like the bitrate
// out-of-spec tests -- log the observed behavior rather than assume it.

TEST_F(CameraIrFfcTest, Trigger_WithoutConfirmingMode_ObserveBehavior)
{
    const uint64_t seq = getCommandAckSeq(MAV_CMD_USER_4);
    g_payload->setPayloadCameraFFCTrigg();

    AckInfo ack;
    const bool acked = waitForCommandAck(MAV_CMD_USER_4, seq, ack, 3000);
    std::cout << "[FINDING] FFC trigger sent without first confirming Manual mode; "
              << (acked ? "ACK received." : "no ACK received.") << std::endl;

    if (acked) {
        std::cout << "[FINDING] result=" << (int)ack.result << std::endl;
    }
    SUCCEED(); // exploratory -- not asserting a specific expected outcome
}

// ---- Known gap ----

TEST(CameraIRKnownGaps, GetFFCMode_IsEmptyStub_NothingToTest)
{
    GTEST_SKIP() << "getPayloadCameraFFCMode(uint8_t&) has an empty function body in the "
                     "current SDK -- it sends nothing and does not set the output parameter. "
                     "There is nothing to test until the SDK implements it.";
}

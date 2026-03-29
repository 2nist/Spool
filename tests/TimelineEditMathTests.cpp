#include <components/ZoneD/TimelineEditMath.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE ("TimelineEditMath trim-start handles seam crossing", "[timeline][edit][math]")
{
    TimelineEditMath::ClipSegment original;
    original.startBeat = 7.5f;
    original.lengthBeats = 1.5f;

    TimelineEditMath::EditRequest request;
    request.mode = TimelineEditMath::EditMode::trimStart;
    request.deltaBeats = 0.75f;
    request.loopBeats = 8.0f;
    request.minLengthBeats = 0.25f;
    request.snapEnabled = false;

    const auto edited = TimelineEditMath::applyEdit (original, request);
    CHECK (edited.startBeat == Catch::Approx (0.25f).margin (0.0001f));
    CHECK (edited.lengthBeats == Catch::Approx (0.75f).margin (0.0001f));
}

TEST_CASE ("TimelineEditMath trim-end handles seam crossing", "[timeline][edit][math]")
{
    TimelineEditMath::ClipSegment original;
    original.startBeat = 7.5f;
    original.lengthBeats = 1.5f;

    TimelineEditMath::EditRequest request;
    request.mode = TimelineEditMath::EditMode::trimEnd;
    request.deltaBeats = -0.5f;
    request.loopBeats = 8.0f;
    request.minLengthBeats = 0.25f;
    request.snapEnabled = false;

    const auto edited = TimelineEditMath::applyEdit (original, request);
    CHECK (edited.startBeat == Catch::Approx (7.5f).margin (0.0001f));
    CHECK (edited.lengthBeats == Catch::Approx (1.0f).margin (0.0001f));
}

TEST_CASE ("TimelineEditMath move supports snap quantization", "[timeline][edit][math]")
{
    TimelineEditMath::ClipSegment original;
    original.startBeat = 1.1f;
    original.lengthBeats = 1.0f;

    TimelineEditMath::EditRequest request;
    request.mode = TimelineEditMath::EditMode::move;
    request.deltaBeats = 0.17f;
    request.loopBeats = 8.0f;
    request.minLengthBeats = 0.25f;
    request.snapStep = 0.25f;
    request.snapEnabled = true;

    REQUIRE (request.snapEnabled);
    CHECK (TimelineEditMath::snapBeat (1.27f, 0.25f) == Catch::Approx (1.25f).margin (0.0001f));
    const auto edited = TimelineEditMath::applyEdit (original, request);
    CHECK (edited.startBeat == Catch::Approx (1.25f).margin (0.0001f));
    CHECK (edited.lengthBeats == Catch::Approx (1.0f).margin (0.0001f));
}

TEST_CASE ("TimelineEditMath split offset resolves inside seam clip", "[timeline][edit][math]")
{
    float splitOffset = 0.0f;
    const bool ok = TimelineEditMath::splitOffsetWithinClip (7.5f, 1.5f, 0.25f, 8.0f, 0.25f, splitOffset);
    REQUIRE (ok);
    CHECK (splitOffset == Catch::Approx (0.75f).margin (0.0001f));
}

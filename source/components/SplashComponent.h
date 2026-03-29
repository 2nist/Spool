#pragma once

#include "../Theme.h"

class SplashComponent : public juce::Component, private juce::Timer
{
public:
    SplashComponent();
    void paint (juce::Graphics& g) override;
    void timerCallback() override;
    void mouseDown (const juce::MouseEvent& e) override;

    std::function<void()> onFinished;

private:
    void finish();

    juce::Array<juce::Image> frames;
    int currentFrameIndex = 0;
    bool finished { false };
    static const int totalFrames = 180; // 3 seconds * 60 fps
    static const int holdFrames = 30;   // Hold for 0.5 seconds at the end
};


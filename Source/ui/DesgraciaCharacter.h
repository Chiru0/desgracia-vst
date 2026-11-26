#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class DesgraciaProcessor;

// The "Desgracia" character: the hand-drawn face, alive.
//  - mouth (jaw) opens with the output level, like it's singing the patch
//  - bounces / tilts on every note-on, blinks, falls asleep when idle
//  - shakes + chromatic split with DRIVE, pixelates with CRUSH
//  - speech bubble for preset names and knob values
class DesgraciaCharacter : public juce::Component, private juce::Timer
{
public:
    explicit DesgraciaCharacter (DesgraciaProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    void say (const juce::String& text, float seconds = 1.6f);
    void setTint (juce::Colour c) { tint = c; }
    void blink() { blinkT = 0.0f; }

private:
    void timerCallback() override;
    void drawFace (juce::Graphics&, juce::Rectangle<float> area);
    void drawScope (juce::Graphics&, juce::Rectangle<float> area);

    DesgraciaProcessor& proc;
    juce::Image face;            // white lines with alpha
    juce::Image pixelBuf;
    juce::Colour tint;

    float time = 0.0f;
    float mouth = 0.0f;
    float bounce = 0.0f, bounceVel = 0.0f;
    float tilt = 0.0f, tiltVel = 0.0f;
    float blinkT = 1.0f, nextBlink = 2.5f;
    float idleTime = 0.0f, sleep = 0.0f;
    float level = 0.0f;
    float drive = 0.0f, crush = 0.0f;
    int lastNoteCounter = 0;
    juce::Random rng;

    juce::String bubble;
    float bubbleTime = 0.0f;
    float scopeCopy[256] {};
    static constexpr int numBubbles = 14;
    float bx[numBubbles] {}, by[numBubbles] {}, bs[numBubbles] {};
    const juce::Colour bgLight { 0xffe9f7ff };
    juce::ColourGradient skyGrad;
    bool useSky = false;
};

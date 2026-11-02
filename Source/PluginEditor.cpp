#include "PluginProcessor.h"
#include "PluginEditor.h"

WiredLook::WiredLook()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff00ff66));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0b1a0f));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff185429));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6fff0));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff00ff66));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff07140b));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6fff0));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff123e1e));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff00ff66));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0c2113));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00ff66));
}

void WiredLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float start, float end, juce::Slider& /*s*/)
{
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(7.0f);
    auto c = b.getCentre();
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.44f;

    // Dark matrix carbon body
    g.setColour(juce::Colour(0xff09140c));
    g.fillEllipse(b);

    // Subtle emerald outer bevel
    g.setColour(juce::Colour(0xff143b1f));
    g.drawEllipse(b, 1.8f);

    // Inactive track arc
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r, r, 0.0f, start, end, true);
    g.setColour(juce::Colour(0xff0e2815));
    g.strokePath(arc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active value arc in Matrix laser phosphor green
    juce::Path value;
    const float currentAngle = start + pos * (end - start);
    value.addCentredArc(c.x, c.y, r, r, 0.0f, start, currentAngle, true);
    g.setColour(juce::Colour(0xff00ff66));
    g.strokePath(value, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Dial pointer needle
    juce::Line<float> pointer(c.x + std::cos(currentAngle) * (r - 7.0f),
                              c.y + std::sin(currentAngle) * (r - 7.0f),
                              c.x + std::cos(currentAngle) * (r - 18.0f),
                              c.y + std::sin(currentAngle) * (r - 18.0f));
    g.setColour(juce::Colour(0xffe6fff0));
    g.drawLine(pointer, 2.5f);

    // Center cap
    g.setColour(juce::Colour(0xff071009));
    g.fillEllipse(c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
    g.setColour(juce::Colour(0xff00ff66));
    g.fillEllipse(c.x - 2.5f, c.y - 2.5f, 5.0f, 5.0f);
}

void WiredLook::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                             int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    auto area = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(1.0f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(area, 5.0f);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(area, 5.0f, 1.2f);

    juce::Path arrow;
    const float ax = (float)buttonX + (float)buttonW * 0.45f;
    const float ay = (float)buttonY + (float)buttonH * 0.48f;
    arrow.startNewSubPath(ax - 4.0f, ay - 2.5f);
    arrow.lineTo(ax, ay + 2.5f);
    arrow.lineTo(ax + 4.0f, ay - 2.5f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(arrow, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void WiredLook::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(juce::Colour(0xff07140b));
    g.setColour(juce::Colour(0xff185429));
    g.drawRect(0, 0, width, height, 1);
}

void WiredLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& bgColour,
                                     bool isMouseOver, bool isButtonDown)
{
    auto area = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(isButtonDown ? juce::Colour(0xff123e1e) : isMouseOver ? juce::Colour(0xff0f2b16) : bgColour);
    g.fillRoundedRectangle(area, 4.0f);

    g.setColour(isMouseOver ? juce::Colour(0xff00ff66) : juce::Colour(0xff1b5c2d));
    g.drawRoundedRectangle(area, 4.0f, 1.2f);
}

WIR3DEditor::WIR3DEditor(WIR3DProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look);
    setSize(750, 450);

    auto setupKnob = [this](juce::Slider& s, const juce::String& name)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
        s.setName(name);
        addAndMakeVisible(s);
    };

    // Main 5 knobs
    setupKnob(bend, "BEND");
    setupKnob(tune, "TUNE");
    setupKnob(speed, "SPEED");
    setupKnob(harmo, "HARMO");
    setupKnob(smile, "SMILE");

    bendAtt.reset(new SliderAttachment(processor.params, "bend", bend));
    tuneAtt.reset(new SliderAttachment(processor.params, "tune", tune));
    speedAtt.reset(new SliderAttachment(processor.params, "speed", speed));
    harmoAtt.reset(new SliderAttachment(processor.params, "harmo", harmo));
    smileAtt.reset(new SliderAttachment(processor.params, "smile", smile));

    // Secondary knobs
    setupKnob(rptMix, "RPT MIX");
    setupKnob(masterFilter, "FILTER");

    rptMixAtt.reset(new SliderAttachment(processor.params, "rptMix", rptMix));
    filterAtt.reset(new SliderAttachment(processor.params, "filter", masterFilter));

    // Repeat division dropdown
    repeatDiv.addItemList({"1/4", "1/8", "1/16", "1/32", "Glitch"}, 1);
    addAndMakeVisible(repeatDiv);
    repeatAtt.reset(new ComboAttachment(processor.params, "repeat", repeatDiv));

    // Key & Scale dropdowns
    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    addAndMakeVisible(key);
    keyAtt.reset(new ComboAttachment(processor.params, "root", key));

    juce::StringArray scaleList;
    for (int s = 0; s < ScaleQuantizer::NumScales; ++s)
        scaleList.add(ScaleQuantizer::getScaleName(s));
    scale.addItemList(scaleList, 1);
    addAndMakeVisible(scale);
    scaleAtt.reset(new ComboAttachment(processor.params, "scale", scale));

    // Vocal Range dropdown
    rangeBox.addItemList({"Wide (All)", "Low (Bass/Bari)", "Mid (Tenor/Alto)", "High (Soprano)"}, 1);
    addAndMakeVisible(rangeBox);
    rangeAtt.reset(new ComboAttachment(processor.params, "range", rangeBox));

    // Preset dropdown & Save button
    auto pNames = processor.getPresetNames();
    for (int i = 0; i < pNames.size(); ++i)
        presetBox.addItem(pNames[i], i + 1);
    presetBox.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    addAndMakeVisible(presetBox);

    presetBox.onChange = [this]()
    {
        processor.loadPreset(presetBox.getSelectedId() - 1);
    };

    addAndMakeVisible(saveBtn);
    saveBtn.onClick = [this]()
    {
        const int curSlot = presetBox.getSelectedId() - 1;
        processor.saveUserPreset(curSlot, "User Preset " + juce::String(curSlot + 1));
        presetBox.changeItemText(curSlot + 1, "User Preset " + juce::String(curSlot + 1));
    };
}

WIR3DEditor::~WIR3DEditor()
{
    setLookAndFeel(nullptr);
}

void WIR3DEditor::paint(juce::Graphics& g)
{
    // Deep terminal background with CRT scanlines
    juce::ColourGradient bg(juce::Colour(0xff09140c), 0, 0, juce::Colour(0xff030704), 750, 450, false);
    g.setGradientFill(bg);
    g.fillAll();

    // Subtle CRT scanlines
    g.setColour(juce::Colour(0x0600ff66));
    for (int y = 0; y < 450; y += 4)
        g.drawHorizontalLine(y, 0.0f, 750.0f);

    // Outer cyber border frame
    g.setColour(juce::Colour(0xff143b1f));
    g.drawRoundedRectangle(12.0f, 12.0f, 726.0f, 426.0f, 14.0f, 1.5f);

    // Lain-inspired cyberpunk mascot:
    // Iconic silhouette with asymmetrical side-lock, cyber wired headset & neon green eye glow
    g.setColour(juce::Colour(0xff0c2415));
    juce::Path hair;
    hair.addEllipse(26.0f, 22.0f, 74.0f, 76.0f);
    // Iconic Lain asymmetrical long side strand
    hair.startNewSubPath(26.0f, 50.0f);
    hair.lineTo(20.0f, 85.0f);
    hair.lineTo(24.0f, 105.0f);
    hair.lineTo(32.0f, 80.0f);
    hair.closeSubPath();
    g.fillPath(hair);

    // Face silhouette
    g.setColour(juce::Colour(0xff184025));
    g.fillEllipse(38.0f, 32.0f, 50.0f, 56.0f);

    // Bangs / Fringe
    g.setColour(juce::Colour(0xff09180e));
    juce::Path fringe;
    fringe.startNewSubPath(38.0f, 48.0f);
    fringe.lineTo(44.0f, 24.0f);
    fringe.lineTo(56.0f, 36.0f);
    fringe.lineTo(69.0f, 22.0f);
    fringe.lineTo(84.0f, 46.0f);
    fringe.lineTo(76.0f, 54.0f);
    fringe.lineTo(68.0f, 43.0f);
    fringe.lineTo(55.0f, 51.0f);
    fringe.lineTo(47.0f, 44.0f);
    fringe.closeSubPath();
    g.fillPath(fringe);

    // Glowing Matrix cyber eyes
    g.setColour(juce::Colour(0xff00ff66));
    g.fillEllipse(49.0f, 56.0f, 5.5f, 7.5f);
    g.fillEllipse(69.0f, 56.0f, 5.5f, 7.5f);
    g.setColour(juce::Colour(0xffe6fff0));
    g.fillEllipse(50.5f, 57.5f, 2.0f, 2.5f);
    g.fillEllipse(70.5f, 57.5f, 2.0f, 2.5f);

    // Headset wire tracing down
    g.setColour(juce::Colour(0xff00ff66));
    juce::Path wire;
    wire.startNewSubPath(33.0f, 62.0f);
    wire.lineTo(28.0f, 75.0f);
    wire.lineTo(31.0f, 105.0f);
    wire.lineTo(50.0f, 115.0f);
    g.strokePath(wire, juce::PathStrokeType(1.5f));

    // Header branding: wir3d
    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(29.0f, juce::Font::bold));
    g.drawText("wir3d", 115, 23, 140, 36, juce::Justification::left);

    g.setColour(juce::Colour(0xff73e699));
    g.setFont(juce::Font(10.5f));
    g.drawText("PROTOCOL 07 // NAVI AUDIO UNIT", 117, 59, 230, 16, juce::Justification::left);

    // Preset label
    g.setColour(juce::Colour(0xff57a871));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("PRESET", 505, 20, 60, 16, juce::Justification::left);

    // Main section separator
    g.setColour(juce::Colour(0xff143b1f));
    g.drawHorizontalLine(115, 20.0f, 730.0f);

    // 5 Main knobs labels & cyber sub-labels
    const juce::String labels[] = {"BEND", "TUNE", "SPEED", "HARMO", "SMILE"};
    const juce::String subLabels[] = {"SHIFT", "SCALE", "SNAP/GLIDE", "PING-PONG", "BLOOM/DRIVE"};
    for (int i = 0; i < 5; ++i)
    {
        const int x = 20 + i * 142;
        g.setColour(juce::Colour(0xffe6fff0));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(labels[i], x, 126, 118, 18, juce::Justification::centred);

        g.setColour(juce::Colour(0xff4ca668));
        g.setFont(juce::Font(9.5f));
        g.drawText(subLabels[i], x, 142, 118, 14, juce::Justification::centred);
    }

    // Lower Dock Panel: Beat Repeat + Master Filter + Quantizer & Range
    auto dock = juce::Rectangle<float>(20.0f, 276.0f, 710.0f, 126.0f);
    g.setColour(juce::Colour(0xff07140b));
    g.fillRoundedRectangle(dock, 8.0f);
    g.setColour(juce::Colour(0xff12381d));
    g.drawRoundedRectangle(dock, 8.0f, 1.2f);

    // Dock Sub-sections Titles
    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(10.5f, juce::Font::bold));
    g.drawText("BEAT REPEAT // GLITCH", 34, 282, 160, 16, juce::Justification::left);
    g.drawText("MASTER FILTER", 235, 282, 130, 16, juce::Justification::left);
    g.drawText("QUANTIZER & VOCAL RANGE", 410, 282, 220, 16, juce::Justification::left);

    // Dock knob labels
    g.setColour(juce::Colour(0xffa3e6b8));
    g.setFont(juce::Font(10.0f));
    g.drawText("RPT MIX", 30, 380, 75, 14, juce::Justification::centred);
    g.drawText("DIVISION", 112, 344, 75, 14, juce::Justification::centred);
    g.drawText("LP  <  0  >  HP", 250, 380, 95, 14, juce::Justification::centred);

    g.drawText("KEY", 410, 344, 45, 14, juce::Justification::centred);
    g.drawText("SCALE", 465, 344, 120, 14, juce::Justification::centred);
    g.drawText("RANGE", 595, 344, 115, 14, juce::Justification::centred);

    // Footer signature
    g.setColour(juce::Colour(0xff2a693b));
    g.setFont(juce::Font(10.0f));
    g.drawText("CONNECTED TO THE WIRED // ALL RESISTANCE IS VOID // COPELAND-OS", 24, 416, 500, 16, juce::Justification::left);
}

void WIR3DEditor::resized()
{
    // Preset dropdown & Save button
    presetBox.setBounds(505, 36, 160, 24);
    saveBtn.setBounds(670, 36, 52, 24);

    // Main 5 knobs
    juce::Slider* mainSliders[] = {&bend, &tune, &speed, &harmo, &smile};
    for (int i = 0; i < 5; ++i)
        mainSliders[i]->setBounds(20 + i * 142, 158, 118, 110);

    // Beat repeat dock controls
    rptMix.setBounds(30, 302, 75, 76);
    repeatDiv.setBounds(112, 317, 75, 24);

    // Master filter knob
    masterFilter.setBounds(260, 302, 75, 76);

    // Key, Scale & Range dropdowns
    key.setBounds(410, 317, 45, 24);
    scale.setBounds(465, 317, 120, 24);
    rangeBox.setBounds(595, 317, 120, 24);
}

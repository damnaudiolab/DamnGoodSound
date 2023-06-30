#pragma once

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel() {
        LookAndFeel::setDefaultLookAndFeel(this);
    }
    ~CustomLookAndFeel() {}

    static const Font getCustomFont()
    {
        static auto typeface = Typeface::createSystemTypefaceFor(BinaryData::BarlowRegular_ttf, BinaryData::BarlowRegular_ttfSize);
        return Font(typeface);
    }

    Typeface::Ptr getTypefaceForFont(const Font&) override
    {
        return getCustomFont().getTypefacePtr();
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
        Slider& slider) override
    {
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto bounds = Rectangle<int>(x, y, width, height).toFloat();
        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto lineW = jmin(16.0f, radius * 0.5f);
        auto knobRadius = radius - lineW * 2;
        auto arcRadius = radius - lineW * 0.5f;

        auto outlineColour = colourPalette[black];
        auto valueFillColour = colourPalette[green];
        auto knobFillColour = colourPalette[white];
        auto thumbColour = colourPalette[black];

        bool sliderPolarity = (slider.getRange().getStart() / slider.getRange().getEnd()) == -1.0;

        Path backgroundArc;
        backgroundArc.addCentredArc(
            bounds.getCentreX(),
            bounds.getCentreY(),
            arcRadius,
            arcRadius,
            0.0f,
            rotaryStartAngle,
            rotaryEndAngle,
            true);
        g.setColour(outlineColour);
        g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::butt));

        Path valueArc;
        valueArc.addCentredArc(
            bounds.getCentreX(),
            bounds.getCentreY(),
            arcRadius,
            arcRadius,
            0.0f,
            sliderPolarity ? 2.0f * float_Pi : rotaryStartAngle,
            toAngle,
            true);
        g.setColour(valueFillColour);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::butt));

        g.setColour(knobFillColour);
        g.fillEllipse(
            bounds.getCentreX() - knobRadius,
            bounds.getCentreY() - knobRadius,
            knobRadius * 2,
            knobRadius * 2);

        auto pointerDistance = knobRadius * 0.8f;
        auto pointerLength = knobRadius * 0.4f;
        Path pointer;
        pointer.startNewSubPath(0.0f, -pointerLength);
        pointer.lineTo(0.0f, -pointerDistance);
        pointer.closeSubPath();
        pointer.applyTransform(AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));
        g.setColour(thumbColour);
        g.strokePath(pointer, PathStrokeType(lineW * 0.5f, PathStrokeType::curved, PathStrokeType::rounded));

    }

    virtual Label* createSliderTextBox(Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox(slider);
        l->setFont(getCustomFont().withHeight(fontSize));
        l->setJustificationType(Justification::centred);
        l->setColour(Label::textWhenEditingColourId, colourPalette[white]);
        l->setColour(Label::backgroundWhenEditingColourId, colourPalette[grey]);
        return l;
    }

    virtual void drawLabel(Graphics& g, Label& label) override
    {
        g.fillAll(label.findColour(Label::backgroundColourId));
        g.setColour(label.findColour(Label::textColourId));
        g.setFont(getCustomFont().withHeight(fontSize));
        g.drawFittedText(label.getText(), label.getLocalBounds(), label.getJustificationType(), 1);
    }
    
    enum colourIdx
    {
        grey,
        black,
        green,
        white,
    };

    Array<Colour> colourPalette = {
        Colour(0xff2a2e33),
        Colour(0xff1e2326),
        Colour(0xff00cc99),
        Colour(0xfff0f8ff),
    };

private:
    //54626f,36454f,00cc99,f0f8ff,91a3b0

    float fontSize = 24.0f;
};
# Vectral

a delay plugin created in Hawaii

## Features

- a knob to adjust mix: this changes the gain of the delayed signal
- a knob to adjust delay time: this changes the time between the input signal and the delay sound
- a knob to adjust the regen: this changes how many times the delay is played
- a button to turn on modulation: an LFO added to the delayed signal

# TODO: Add to notes
https://forum.juce.com/t/simple-lfo-on-amplitude/52490

Left off with this one: https://www.youtube.com/watch?v=YJ4YbV6TDo0

Left off here for delay line example: https://www.youtube.com/watch?v=sE5bH0kf1eM

Building the UI in React: https://youtu.be/vxCTwNi2BXg?si=lwZQsuCAK0rYVjsM
Left off at 24:22

## Implementation Notes

- Rather than using the Projucer (JUCE's configuration utility), this project uses cmake. The cmake setup is bootstrapped by Pamplejuce, which is a JUCE CI template for GitHub actions.
- Rather than using JUCE's GUI capabilities, the GUI will be written in React. This will allow the project to take advantage of declarative UI and modern CSS libraries.
- Here is an example of a JS Web Component in CMajor: https://github.com/SoundStacks/cmajor/blob/main/examples/patches/CustomGUI/demo_ui/index.js
After watching the above talk about building the UI in React, I'm more interested in CMajor than react-juce because of better maintenance.

### CMajor Integration

This project has the DSP logic written in JUCE C++ and the UI logic written in JavaScript via CMajor. The UI pretty much embeds a webview in the application.

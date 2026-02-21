## Description

Please include a summary of the change and which issue is fixed.

Fixes # (issue)

## Type of change

- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## How Has This Been Tested?

Please describe the tests that you ran to verify your changes.

- [ ] Tested on actual hardware
- [ ] ESP32 boots without errors
- [ ] CrowPanel display turns on, shows waiting state
- [ ] ESP-NOW compass data received, heading and compass rose update correctly
- [ ] Connected indicator (red dot) disappears when data flows
- [ ] Heading mode toggle works on CompassScreen (T/M)
- [ ] Screen carousel works with rotary knob (CW and CCW)
- [ ] AttitudeScreen updates correctly
- [ ] Attitude leveling works with two-press confirmation
- [ ] BrightnessScreen adjustment works and persists after reboot (NVS)
- [ ] No memory leaks observed (Serial [DIAG] output)
- [ ] No unexpected bottlenecks in [DIAG] output

**Test Configuration**:
- Arduino IDE: [e.g. 2.3.6]
- ESP32 board package: [e.g. 2.0.14]
- LVGL: [e.g. 8.3.11]
- SquareLine Studio: [e.g. 1.6.0]

## Checklist:

- [ ] My code follows the style guidelines of this project
- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have tested on actual hardware if possible
- [ ] I have acknowledged any AI assistance in code comments/PR description

## AI Assistance

- [ ] No AI tools used
- [ ] ChatGPT used for code generation/review
- [ ] Claude used for code generation/review
- [ ] GitHub Copilot used
- [ ] Other AI tools (specify): _____________

If AI was used, describe the prompts and how you verified the code:
```
(describe here)
```


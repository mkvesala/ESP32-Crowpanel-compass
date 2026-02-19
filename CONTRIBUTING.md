# Contributing to ESP32-CrowPanel-compass

Thank you for considering contributing to this project! This document provides guidelines for contributing.

## How Can I Contribute?

### About the project

This is an individual *learning project* by me. Please refer to [README.md](README.md) for the purpose of the project. While new feature suggestions and bug fixes are welcome, the most precious are any advice that might help the me improving my knowledge in ESP32 programming.

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates. When creating a bug report, include:

- Clear description of the issue
- Detailed steps to reproduce the behavior
- What you expected to happen
- What actually happened
- Environment:
  - Arduino IDE version
  - ESP32 board package version
  - Hardware used
  - Library versions
- Output (Serial monitor, CrowPanel display)
- Screenshots, if applicable

### Suggesting Features

Feature requests are welcome! Please:

- Use a clear and descriptive title
- Provide detailed description of the proposed feature
- Explain why this feature would be useful
- Include code examples or mockups if applicable

### Pull Requests

1. **Fork the repository** and create your branch from `main`
   ```
   git checkout -b feature/your-feature-name
   ```
2. Follow the coding style used in the project:
   - Use descriptive variable and function names in English
   - Add comments for complex logic in English
   - Keep class responsibilities clear and focused
   - Follow existing naming conventions
3. Test your changes:
   - Compiles without errors or warnings
   - Test on actual hardware, out there if possible
   - Check that existing features work
4. Update documentation:
   - Update README.md if you add new features
5. Commit your changes:
   ```
   git commit -m "Add brief description of your changes"
   ```
   - Use clear commit messages
6. Push to your fork and submit a PR:
   ```
   git push origin feature/your-feature-name
   ```
7. Wait for maintainer's review and feedback

## Coding guidelines

1. Use consistent indentation
2. Opening brace on the same line for functions and control structures:
  ```
   void myFunction() {
      if (condition) {
         // code
      }
   }
   ```
3. No braces if one line of code like in:
   ```
   if (condition) return;
   ```
4. Naming:
   - Classes `PascalCase`
   - Functions/methods `camelCase`
   - Variables `camelCase` or `snake_case` - be consistent
   - Constants `UPPER_SNAKE_CASE`
   - Timers: use `_ms`, `_MS`, `_us` to indicate the units for example `NEXT_TRY_MS`
   - Private members starting with `_underscore`
5. Use `//` in single line comments
6. Avoid `#DEFINE` macros, use `const` and `static constexpr` where appropriate
7. Avoid String variables
8. Use `delay()` in loop task *only* when absolutely necessary
9. Minimize dynamic memory allocation - prefer stack allocation
12. Respect ESP32 limited resources and core 0/core 1 task responsibilities if such applied

## Testing checklist

* [ ] ESP32 boots without errors
* [ ] Crowpanel display turns on promptly, into "waiting for data" state
* [ ] Compass data received, heading label and compass rose update correctly
* [ ] Connected-indicator (the red dot on the compass screen) disappears
* [ ] Heading mode toggle works when knob button pressed on compass screen
* [ ] Screen carousel works fine with rotary knob
* [ ] Attitude screen updates correctly
* [ ] Attitude Leveling works promptly with knob press on attitude screen
* [ ] Success-response received and displayed after leveling
* [ ] Brightness screen updates correctly with value stored in NVS
* [ ] Brightness adjustment works promptly on knob press/rotate
* [ ] No memory leaks (monitor serial)
* [ ] Diagnostics do not report bad bottlenecks (monitor serial).

## Development environment

### Software requirements

[README.md](README.md) for details.

### Hardware requirements

[README.md](README.md) for details.

## AI-assisted development

### Policy

1. **Using AI tools is allowed and encouraged** - ChatGPT, Claude Code, Github Copilot or IDE AI copilots, whatever works for you
2. **Transparency is important**  - be open and mention in code comments, commit messages and PRs that AI has assisted you in development
3. **Understanding first** - you are responsible for not only the code itself but also being able to explain it

### Best practises

1. **Always test** - AI-assisted code requires especially careful testing
2. **Check security** - ensure that there are no vulnerabilities
3. **Match style** - verify that the code follows the project's conventions and coding guidelines
4. **Share learnings** - if AI helped to to solve a problem, share it in comments, PRs or discussions
5. **Focus on prompting** - at least prompt: *role, task, tone, audience, restrictions/limits/guardrails, output format, output verification, list of sources used*
6. **Use different tools and prompts** - prompt several times, with different AI tools, different day, seek cross-verification

### Avoid

1. **Copy-pasting code without understanding what it does**
2. **Blindly trusting AI tools without validation**
3. **Shifting responsiblitity to AI - you are the author**

### Copyright

AI tools are assitive tools. You are responsible for the code. Make your best to ensure that the code does not violate 3rd party licenses.

## Code of conduct

1. Check existing issues
2. Create a new issue with your question
3. Be patient - I'm maintaining this on my freetime among other boat projects
4. Be respectful and constructive
5. Welcome beginners
7. No harrassment, no inappropriate behaviour, no crimes




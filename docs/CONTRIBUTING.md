# Contributing to libjpeg

Thank you for your interest in contributing to this project! This document provides guidelines for contributing.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help maintain a welcoming environment

## How to Contribute

### Reporting Bugs

When reporting bugs, please include:
- Operating system and architecture
- Compiler version
- Steps to reproduce the issue
- Expected vs actual behavior
- Sample code if applicable

### Suggesting Enhancements

Enhancement suggestions are welcome! Please:
- Clearly describe the feature
- Explain the use case
- Consider backward compatibility
- Discuss performance implications

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Add tests if applicable
5. Ensure code compiles without warnings
6. Update documentation
7. Commit with clear messages (`git commit -m 'Add amazing feature'`)
8. Push to your branch (`git push origin feature/amazing-feature`)
9. Open a Pull Request

## Coding Standards

### C Code Style

- Use C99 standard
- 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- Use descriptive variable names
- Add comments for complex logic
- Follow existing code style

### Documentation

- Document all public APIs with Doxygen-style comments
- Include parameter descriptions
- Specify return values
- Note any side effects or requirements
- Provide usage examples

### SIMD Code

- Use conditional compilation for architecture-specific code
- Provide scalar fallback implementations
- Test on target architectures when possible
- Document SIMD intrinsics usage

## Testing

- Test on multiple platforms if possible
- Verify output JPEG files with standard viewers
- Check for memory leaks with valgrind
- Benchmark performance changes

## Commit Messages

- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit first line to 72 characters
- Reference issues and pull requests

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.

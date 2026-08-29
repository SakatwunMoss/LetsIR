#pragma once

#include <juce_core/juce_core.h>

#include <string>

/** Build a juce::String from a UTF-8 narrow literal / buffer.
    juce::String(const char*) treats input as ASCII and corrupts multi-byte text. */
inline juce::String utf8 (const char* text)
{
    return juce::String (juce::CharPointer_UTF8 (text != nullptr ? text : ""));
}

inline juce::String utf8 (const std::string& text)
{
    return juce::String::fromUTF8 (text.c_str(), static_cast<int> (text.size()));
}

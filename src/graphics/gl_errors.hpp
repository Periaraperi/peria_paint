#pragma once

#include <glad/glad.h>
#include <print>

namespace peria::gl {

inline APIENTRY void debug_callback(GLenum source, GLenum type, unsigned int id, GLenum severity, 
                                    [[maybe_unused]]GLsizei length, const char *message, [[maybe_unused]]const void *userParam)
{
    // these error codes can be ignored.
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    std::println("--------------------------------------------------------------------");
    std::println("Debug message ({}): {}", id, message);

    switch (source) {
        case GL_DEBUG_SOURCE_API:             std::print("Source: API");             break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::print("Source: Window System");   break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::print("Source: Shader Compiler"); break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::print("Source: Third Party");     break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::print("Source: Application");     break;
        case GL_DEBUG_SOURCE_OTHER:           std::print("Source: Other");           break;
        default:                              std::print("SHOULDN'T BE HERE");       break; 
    }
    std::println();

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               std::print("Type: Error");                break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::print("Type: Deprecated Behaviour"); break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::print("Type: Undefined Behaviour");  break; 
        case GL_DEBUG_TYPE_PORTABILITY:         std::print("Type: Portability");          break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::print("Type: Performance");          break;
        case GL_DEBUG_TYPE_MARKER:              std::print("Type: Marker");               break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::print("Type: Push Group");           break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::print("Type: Pop Group");            break;
        case GL_DEBUG_TYPE_OTHER:               std::print("Type: Other");                break;
        default:                                std::print("SHOULDN'T BE HERE");          break; 
    }
    std::println();
    
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         std::print("Severity: high");         break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::print("Severity: medium");       break;
        case GL_DEBUG_SEVERITY_LOW:          std::print("Severity: low");          break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::print("Severity: notification"); break;
    }
    std::println();
    std::println("--------------------------------------------------------------------");
}


}

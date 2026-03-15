#ifndef TOYMAKERTEST_STRINGCONVERSIONS_H
#define TOYMAKERTEST_STRINGCONVERSIONS_H

#include <doctest/doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>


namespace doctest {
    template<> struct StringMaker<glm::vec3> {
        static String convert(const glm::vec3& value) {
            std::stringstream stringStream {};
            stringStream << "vec3(" << value.x << ", " <<
                value.y << ", " << value.z << ")";
            return String{ stringStream.str().c_str() };
        }
    };

    template<> struct StringMaker<glm::quat> {
        static String convert(const glm::quat& value) {
            std::stringstream stringStream {};
            stringStream << "quat(w = " << value.w <<
                ", xyz = " << value.x << ", " << value.y
                << ", " << value.z << ")";
            return String{ stringStream.str().c_str() };
        }
    };
}

#endif

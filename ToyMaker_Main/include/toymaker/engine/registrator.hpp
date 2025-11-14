/**
 * @ingroup ToyMakerCore
 * @file registrator.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the definition for the Registrator<T> utility class, used anywhere that automatic registration of some kind during the static initialization phase of a program is required.
 * @version 0.3.2
 * @date 2025-09-06
 * 
 * 
 */

#ifndef FOOLSENGINE_REGISTRATOR_H
#define FOOLSENGINE_REGISTRATOR_H

namespace ToyMaker {

    /**
     * @ingroup ToyMakerCore
     * @brief Helper class for registering a class at program startup.
     *
     * It accomplishes this by:
     *
     * - Forcing an implementation of static function `registerSelf()` using CRTP, called here.
     *
     * - Ensuring its owner's `registerSelf()` function is called during the static initialization phase of the program.
     *
     * ## Usage:
     *
     * ```c++
     *
     * class YourClass {
     *
     *     // ... the rest of your class definition
     *     // ...
     *
     *     YourClass() {s_registrator.emptyFunc()} // NOTE: Explicit constructor definition
     *
     *     // NOTE: This function will be called by constructor Registrator<YourClass>()
     *     static YourReturnType registerSelf() {
     *
     *         // NOTE: Ensure correct order of registration for related classes,
     *         // hopefully avoiding static initialization order fiasco
     * 
     *         Registrator<ClassYouDependOn>::getRegistrator(); //< Calls ClassYouDependOn's registerSelf() in turn.
     *         Registrator<AnotherClassYouDependOn>::getRegistrator();
     *
     *         // NOTE: ... whatever the class needs to do to actually register itself
     *         // wherever it needs to be registered
     *
     *     }
     *
     *     // NOTE: Declaration and initialization of the registrator static
     *     // member associated with your class.
     *     inline static Registrator<YourClass> s_registrator { Registrator<YourClass>::getRegistrator() };
     *
     *     // ... 
     *     // The rest of your class definition
     *
     * };
     *
     * ```
    */
    template<typename TRegisterable>
    class Registrator {
    public:
        inline static Registrator& getRegistrator() {
            static Registrator reg {};
            return reg;
        };
        void emptyFunc() {}
    protected:
        Registrator();
    };

    template <typename TRegisterable>
    Registrator<TRegisterable>::Registrator() {
        TRegisterable::registerSelf();
    }

}

#endif

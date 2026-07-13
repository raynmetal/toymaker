/**
 * @ingroup Examples
 * @file spin_on_click.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the class responsible for applying torque to the central text object
 * when it is clicked.
 * @version 0.2.3
 * @date 2026-06-13
 * 
 */

#ifndef TOYMAKEREXAMPLE_SPINONCLICK_H
#define TOYMAKEREXAMPLE_SPINONCLICK_H

#include <toymaker/engine/sim_system.hpp>
#include <toymaker/builtins/interface_pointer_callback.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect responsible for applying torque to the object it is attached to when clicked,
 * depending on the location clicked.
 */
class SpinOnClick: public ToyMaker::SimObjectAspect<SpinOnClick>, public ToyMaker::ILeftClickable {
public:
    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "SpinOnClick"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "SpinOnClick" }
     * ```
     *
     * @param jsonAspectProperties This aspect's description in JSON.
     *
     * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
     *
     */
    static std::shared_ptr<ToyMaker::BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

    /**
     * @brief Uses this aspect's data to construct a new aspect.
     *
     * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
     *
     */
    std::shared_ptr<ToyMaker::BaseSimObjectAspect> clone() const override;

    /**
     *
     * @brief Responds to left click events by applying a torque to the object
     * it is attached to.
     *
     * @param clickLocation The 3D coordinates of the location on parent object that was
     * clicked.
     *
     * @retval true The aspect consumed the click.
     * @retval false The aspect did not consume the click
     */
    bool onPointerLeftClick(glm::vec4 clickLocation) override;

    /**
     * @brief Unused pointer release implementation
     *
     * @retval true The aspect consumed the click
     * @retval false The aspect did not consume the click
     */
    bool onPointerLeftRelease(glm::vec4 clickLocation) override {
        (void) clickLocation; // prevent unused parameter warnings
        return false;
    }

private:
    /**
     * @brief Constructs a new Spin On Click object.
     *
     */
    SpinOnClick(): ToyMaker::SimObjectAspect<SpinOnClick>{0} {}
};

#endif


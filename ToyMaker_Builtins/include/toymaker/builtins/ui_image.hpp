/**
 * @ingroup UrGameUIComponent
 * @file ui_image.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the definition of the aspect responsible for the model-texture combo for displaying an image from a file.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 * 
 */

#ifndef ZOAPPUIIMAGE_H
#define ZOAPPUIIMAGE_H

#include "toymaker/engine/sim_system.hpp"
#include "toymaker/engine/texture.hpp"

/**
 * @ingroup UrGameUIComponent
 * @brief The aspect class responsible for displaying an image from a file scaled to some specific dimensions.
 * 
 * ## Usage:
 * 
 * Its appearance in JSON is as follows:
 * 
 * ```jsonc
 * 
 * {
 *     "type": "UIImage",
 *     "image_filepath": "data/textures/button_active.png",
 *     "dimensions": [620, 440],
 *     "anchor": [0.5, 0.5]
 * }
 * 
 * ```
 * 
 * To change the image associated with this aspect, call UIImage::updateImage(), passing the path to the file from which the new image is to be loaded.
 * 
 */
class UIImage: public ToyMaker::SimObjectAspect<UIImage> {
public:
    /**
     * @brief Constructs a new UIImage aspect.
     * 
     */
    UIImage(): SimObjectAspect<UIImage>{0} {}

    /**
     * @brief Gets the aspect type string associated with this class.
     * 
     * @return std::string This class' aspect type string.
     */
    inline static std::string getSimObjectAspectTypeName() { return "UIImage"; }

    /**
     * @brief Creates a new UIImage instance from its description in JSON.
     * 
     * @param jsonAspectProperties The JSON description of the UIImage instance.
     * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIImage object.
     */
    static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

    /**
     * @brief Constructs a new UIImage using this object as its blueprint.
     * 
     * @return std::shared_ptr<BaseSimObjectAspect> A pointer to the base aspect class of the new UIImage object.
     */
    std::shared_ptr<BaseSimObjectAspect> clone() const override;

    /**
     * @brief Loads and displays the image associated with this aspect when it is added to the active scene.
     * 
     */
    void onActivated() override;

    /**
     * @brief Updates the image associated with this object.
     * 
     * @param imageFilepath The path to the file containing the new image to be displayed.
     */
    void updateImage(const std::string& imageFilepath);

    /**
     * @brief Changes the dimensions the loaded image should be confined or scaled to (while maintaining its original aspect).
     * 
     * @param dimensions The new dimensions of the image.
     */
    void updateDimensions(const glm::uvec2& dimensions);

    /**
     * @brief Updates the point considered the origin of this object's StaticModel.
     * 
     * (0, 0) corresponds to the top left corner of this object, while (1, 1) corresponds to the bottom right corner.
     * 
     * @param anchor The new origin of the object, relative to its top left corner and the objects total dimensions.
     */
    void updateAnchor(const glm::vec2& anchor);

private:
    /**
     * @brief Recomputes the texture associated with this object's StaticModel.
     * 
     */
    void recomputeTexture();

    /**
     * @brief The file from which the image to be displayed was loaded.
     * 
     */
    std::string mImageFilepath {};

    /**
     * @brief The point considered the origin of this object, with (0,0) being its top left corner and (1,1) being its bottom right corner.
     * 
     */
    glm::vec2 mAnchor {0.f, 0.f};

    /**
     * @brief The dimensions within which the loaded image will be displayed.
     * 
     */
    glm::uvec2 mDimensions { 0.f, 0.f };
};

#endif

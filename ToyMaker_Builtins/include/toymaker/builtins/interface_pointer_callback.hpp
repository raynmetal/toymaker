/**
 * @ingroup UrGameInteractionLayer
 * @file interface_pointer_callback.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains classes that serve as interfaces for sim objects that wish to respond to click events.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 */

/**
 * @defgroup UrGameInteractionLayer Interaction Layer
 * @ingroup UrGame
 * 
 */

#ifndef ZOAPPINTERFACEPOINTERCALLBACK_H
#define ZOAPPINTERFACEPOINTERCALLBACK_H

namespace ToyMaker {
    class IRightClickable;
    class ILeftClickable;
    class IHoverable;

    /**
     * @ingroup UrGameInteractionLayer
     * @brief The interface used by aspects, objects, which cast pointer rays to interact with objects present in the world.
     * 
     * This interface will usually be attached to an aspect associated with a camera object.
     * 
     * Contains methods to invoke pointer event callbacks on objects that implement IRightClickable, ILeftClickable, and IHoverable.
     * 
     * The responsibility of actually conducting the raycast falls on the aspect implementing this interface itself.
     * 
     */
    class IUsePointer {
    public:
        /**
         * @brief Virtual IUsePointer destructor, so that subclasses can use their own.
         * 
         */
        ~IUsePointer()=default;
    protected:
        /**
         * @brief Calls the left mouse button press (or equivalent) callback on an object.
         * 
         * @param clickable The aspect with a left click callback.
         * @param clickLocation The 3D world coordinates where the object was clicked.
         * @retval true The object handled the left mouse button press.
         * @retval false The object did not handle the left mouse button press.
         */
        bool leftClickOn(ILeftClickable& clickable, glm::vec4 clickLocation);

        /**
         * @brief Calls the left mouse button release (or equivalent) callback on an object.
         * 
         * @param clickable The aspect with the left click callback.
         * @param clickLocation The 3D world coordinates where the object was clicked.
         * @retval true The object handled the left mouse button release.
         * @retval false The object did nothing with the left mouse button release.
         */
        bool leftReleaseOn(ILeftClickable& clickable, glm::vec4 clickLocation);

        /**
         * @brief Callback for when a pointer enters a region containing an object.
         * 
         * @param hoverable The aspect with the hover callback.
         * @param hoverLocation The location at which the pointer entered the object's region.
         * @retval true The object handled the pointer enter event.
         * @retval false The object did nothing with the pointer enter event.
         */
        bool pointerEnter(IHoverable& hoverable, glm::vec4 hoverLocation);

        /**
         * @brief Callback for when a pointer leaves a region containing an object.
         * 
         * @param hoverable The aspect with the hover callback.
         * @retval true The object handled the pointer leave event.
         * @retval false The object did nothing with the pointer leaving event.
         */
        bool pointerLeave(IHoverable& hoverable);
    };

    /**
     * @ingroup UrGameInteractionLayer
     * @brief The interface used by aspects which wish to respond to mouse left click events (or equivalent).
     * 
     */
    class ILeftClickable {
        /**
         * @brief Virtual method called by a subclass of IUsePointer to signal that a left mousebutton press event has occurred.
         * 
         * @param clickLocation The 3D coordinates at which the left click event occurred.
         * @retval true Returned when the left click event is handled by the implementer.
         * @retval false Returned when nothing is done with the left click event.
         */
        virtual bool onPointerLeftClick(glm::vec4 clickLocation)=0;

        /**
         * @brief Virtual method called by a subclass of IUsePointer to signal that a left mousebutton release event has occurred.
         * 
         * @param clickLocation The 3D coordinates at which the left mouse button release occurred.
         * @retval true Returned when the left click event is handled by the implementer.
         * @retval false Reeturned when nothing is done with the left click event.
         */
        virtual bool onPointerLeftRelease(glm::vec4 clickLocation)=0;
    friend class IUsePointer;
    };

    // class IRightClickable {
    //     virtual bool onPointerRightClick(glm::vec4 clickLocation)=0;
    //     virtual bool onPointerRightRelease(glm::vec4 clickLocation)=0;
    // friend class IUsePointer;
    // };

    /**
     * @ingroup UrGameInteractionLayer
     * @brief The interface implemented by aspects which wish to respond to pointer hover related events.
     * 
     */
    class IHoverable {
        /**
         * @brief Virtual method called by a subclass of IUsePointer to signal that a pointer has entered this object's region.
         * 
         * @param hoverLocation The 3D coordinates at which the pointer entered this object's region.
         * @retval true Returned when this object uses the pointer hover event.
         * @retval false Returned when this object does nothing with the pointer hover event.
         */
        virtual bool onPointerEnter(glm::vec4 hoverLocation)=0;

        /**
         * @brief Virtual method called by a subclass of IUsePointer to signal that a pointer has just left this object's region
         * 
         * @retval true Returned when this object uses the pointer hover event.
         * @retval false Returned when this object does nothing with the pointer hover event.
         */
        virtual bool onPointerLeave()=0;
    friend class IUsePointer;
    };

    // inline bool IUsePointer::rightClickOn(IRightClickable& clickable, glm::vec4 clickLocation) { return clickable.onPointerRightClick(clickLocation); }
    // inline bool IUsePointer::rightReleaseOn(IRightClickable& clickable, glm::vec4 clickLocation) { return clickable.onPointerRightRelease(clickLocation); }

    inline bool IUsePointer::leftClickOn(ILeftClickable& clickable, glm::vec4 clickLocation) { return clickable.onPointerLeftClick(clickLocation); }
    inline bool IUsePointer::leftReleaseOn(ILeftClickable& clickable, glm::vec4 clickLocation) { return clickable.onPointerLeftRelease(clickLocation); }

    inline bool IUsePointer::pointerEnter(IHoverable& hoverable, glm::vec4 hoverLocation) { return hoverable.onPointerEnter(hoverLocation); }
    inline bool IUsePointer::pointerLeave(IHoverable& hoverable) { return hoverable.onPointerLeave(); }

}

#endif

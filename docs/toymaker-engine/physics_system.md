# ToyMaker: Physics System

## What is it?

The ToyMaker::PhysicsSystem is responsible for applying physics updates each simulation frame to all eligible entities, especially
those that are tagged with ToyMaker::PhysicsState::MODE_DYNAMIC.  Additionally, it watches for and reports
collision events detected between physics entities during each simulation frame.

In order for an entity to be eligible for the physics system, it _must_ hold the
following components:

- ToyMaker::ObjectBounds -- Data defining a volume on which the physics system
will operate.

- ToyMaker::AxisAlignedBounds -- Derived from ToyMaker::ObjectBounds, this is used primarily
during the physics systems broad phase collision detection stage to limit the number
of collision checks performed.

- ToyMaker::PhysicsState -- A collection of properties defining the behaviour of
the object during physics updates.

## Important API

Mainly, aside from setting object positions through ToyMaker::ObjectBounds rather than
ToyMaker::Placement, one interacts with the physics system through the ToyMaker::PhysicsState component.


- ToyMaker::PhysicsState::mTraits -- Which kind of physics updates(kinematic, dynamic) this object is
subscribed to, and how it responds to collision events.

    - ToyMaker::PhysicsState::MODE_DYNAMIC -- This object responds to both position updates derived from its
    velocity _and_ velocity updates derived from external forces.

    - ToyMaker::PhysicsState::MODE_KINEMATIC -- This object does not respond to velocity updates through forces
    but does update object position & orientation based on velocity.

    - ToyMaker::PhysicsState::MODE_STATIC -- This object doesn't respond to velocity or force updates, and
    is expected to stay in place more or less throughout its life.

    - ToyMaker::PhysicsState::COLLISION_SEPARATE -- This object is configured to separate from other objects
    when collisions are detected.

    - ToyMaker::PhysicsState::COLLISION_SIGNAL -- This object will signal on collision with or separation
    from other objects.

- ToyMaker::PhysicsState::mMassInverse & ToyMaker::PhysicsState::mRotationalInertiaInverse -- Properties defining
the resistance of this object to translational and rotational motion.

- ToyMaker::PhysicsState::mForce & ToyMaker::PhysicsState::mTorque -- The sum external translational force and
torque acting on this object for this physics update.

- ToyMaker::PhysicsState::mVelocity & ToyMaker::PhysicsState::mAngularVelocity -- The current velocity
of this object, in terms of translation and rotation respectively.

- ToyMaker::PhysicsState::mCoefficientFrictionDynamic & ToyMaker::PhysicsState::mCoefficientFrictionStatic -- The object's
resistance to motion relative to another object at its point of contact when
it is moving and when it is stationary respectively.

- ToyMaker::PhysicsState::mCoefficientRestitution -- The total kinetic energy preserved by
two colliding objects after the collision as a fraction of their kinetic energy before
collision.

Practical examples of this system may be found under `Examples/`.

## Why does it exist?

It enables the creation of games and applications that require more complex
physical interactions between objects.


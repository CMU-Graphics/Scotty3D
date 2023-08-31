# `A4T4` Particle Systems

**Implement:**
- `bool Particle::update` in `src/scene/particles.cpp`, which updates a single particle's position over time.

A particle system in Scotty3D is a collection of non-self-interacting, physics-simulated, spherical particles that interact with the rest of the scene. Take a look at the [slightly outdated] [user guide](https://cmu-graphics.github.io/Scotty3D-docs/guide/simulate_mode/) for an overview of how to create and manage them. 

Note that this task is dependent on (at the very least) having a working triangle intersection algorithm (from A3.0). If you would like us to grade with the reference intersection code instead of your intersection code, **make sure to specify this in your writeup**.

## Kinematics 

Each particle is described by a position and a velocity. If we want to know where the particle will be at some future time, we simply have to move it along its velocity vector. If the particle is travelling at constant velocity, this is a very easy expression to compute. 

However, if the particle can have arbitrary forces applied to it, we no longer know exactly how it will move. Since we're interested in physical simulation, we can apply the basic equation governing motion: $F = ma$. Since acceleration is the time derivative of velocity, we now have a differential equation governing how our particle moves. Unfortunately, it's no longer feasible (in general) to compute this motion analytically, so instead we will use numerical integration to step our particles forward in time. To get realistic behavior from approximate but easy to compute steps, we can simply take many small time steps until we reach the desired destination.

$$\frac{dv}{dt} = \frac{F}{m}$$

$$\frac{dx}{dt} = v$$

There are many different techniques for integrating our equations of motion, including forward, backward, and symplectic Euler, Verlet, Runge-Kutta, and Leapfrog. Each strategy comes with a slightly different way of computing how much to update our velocity and position across a single time step. In this case, we will use the simplest -- forward Euler -- as we aren't too concerned with stability or energy conservation. Forward Euler steps position forward by velocity, and then velocity by acceleration:

$$x_{t+\Delta t} = x_t + v_t \cdot \Delta T$$

$$v_{t+\Delta t} = v_t + a \cdot \Delta T$$

In `Particle::update`, use this integrator to step the current particle forward for `dt` seconds. Note that the only force we will apply to our particles is gravity (the `gravity` parameter). Once you've added the update rule, you should already see some interesting behavior - create a particle system and see how the particles travel along nice parabolic trajectories. We've provided a sample one in `media/js3d/A4-particles.js3d`.

## Collisions

The more substantial part of this task is colliding each particle with the rest of our scene geometry. Thankfully, you've already done most of the work required here for A3: we can use Scotty3D's ray-tracing capabilities to find collisions. 

During each timestep, we know that in the absence of a collision, our particle will travel in the direction of velocity for distance `|velocity| * dt`. Hence, we can create a ray from the particle's position and velocity to look for collisions during the time step. If the ray intersects with the scene, we can compute exactly when the particle would experience a collision. 

When collision is found, we could just place the particle at the collision point and be done, but we don't want our particles to simply stick the surface! Instead, we will assume all particles collide elastically (and massless-ly): the magnitude of a particle's velocity should be the same before and after a collision, but its direction should be reflected about the normal of the collided surface. 

Once we have reflected velocity, we can compute how much of the time step remains after the collision. But what if the particle collided with the scene _again_ before the end of the time step? If we are using very small time steps, it might be acceptable to ignore this possibility, but we want to resolve all collisions. So, we can repeat the ray-casting procedure in a loop until we have used up the entire time step, up to some epsilon. Remember to only use the remaining portion of the time step each iteration, and to step forward both the velocity and position at each substep.

### Spherical Particles

The above procedure is perfectly realistic for point particles, but we would like to draw our particles with some non-zero size and still have the simulation appear natural. 

We will hence represent particles as small spheres. This means you must take `radius` into account when finding intersections: if the collision ray intersects a surface, at what distance does the closest point on the sphere incur the collision? (Hint - it depends on the angle of intersection.)

Simply finding closer intersections based on `radius` will not, of course, resolve all sphere-scene intersections: the ray can miss all geometry while the edge of the sphere would see a collision. However, this will produce visually acceptable results, greatly reducing overlap and never letting particles 'leak' through geometry. 

<p align="center">
    <img src="T4/collision.png">
</p>

Once you have got collisions working, you should be able to open `media/js3d/A4-particles.js3d` and see a randomized collision-fueled waterfall. Try rendering the scene!

Tips:
- When accounting for radius, you don't know how far along the ray a collision might occur. Look for a collision at any distance, and if it occurs after the end of the current timestep, ignore it.
- When accounting for radius, consider in what situation(s) you could find a collision at a negative time. In this case, you should not move the particle -- just reflect the velocity outward.
- When accounting for radius, note that you don't necessarily know which direction the normal of the scene intersection is -- if you're trying to subtract a distance to compute where to position the particle to not allow leakage, make sure your distance is positive!
- Particle positions are in world space (not particle-system-local space). This allows particle systems to behave reasonably when emitters' transforms are animated. (And makes it much easier to figure out which rays to construct for collision!)
- We've also provided some test cases, but similar to the other tasks, they are very simple. We encourage using the GUI to test particle simulation as it is much more useful for visualizing what is happening with your code vs outputing a position and velocity of a particle.

## Lifetime

Finally, note that `Particle::update` is supposed to return `false` when the particle should be removed.
Each particle has an `age` member that represents the remaining time it has before removal. Each time step, your code should subtract `dt` from `age` and return whether `age` is still greater than zero. 


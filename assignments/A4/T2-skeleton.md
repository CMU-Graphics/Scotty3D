# `A4T2` Skeleton Kinematics

A `Skeleton` (declared in `src/scene/skeleton.h`, defined in `src/scene/skeleton.cpp`) is what we use to drive our animation. You can think of them like the set of bones we have in our own bodies and joints that connect these bones. For convenience, we have merged the bones and joints into the `Bone` class which holds the orientation of the bone relative to its parent as Euler angles (`Bone::pose`), and an `extent` that specifies where its child bones start. Each `Skinned_Mesh` has an associated `Skeleton` class which holds a rooted tree of `Bones`s, where each `Bone` can have an arbitrary number of children.

When discussing skeletal animation we have a whole family of different transformations and spaces to consider:
- **World space** is the space of the scene.
- **Local space** is the coordinate system local to the `Skinned_Mesh`. It is shared by the `Skinned_Mesh::mesh` and the `Skinned_Mesh::skeleton`. The transformation, $L$, between local space and world space is determined by the scene graph and can be retrieved from `Instance::Skinned_Mesh::transform->local_to_world()` (which you wrote way back in A1!).
- Bone space has its origin at the base point of the bone and its axes rotated by the bone (and its parents) rotations. There are two different bone spaces we will talk about:
	- **Bind Space** is the bone space in the bind pose (the pose where all joints are not rotated). The transformations, $B_j$, from bone space to local space for all bones $j$ is computed by `Skeleton::bind_pose()` (which you will implement).
	- **Pose Space** is the bone space in the current pose (the pose where each bone and its children are rotated by `Bone::pose` around `Bone::compute_rotation_axes`). The transformations, $P_j$, from pose space to local space for all bones $j$ are computed by `Skeleton::current_pose()` (which you will also implement).

*NOTE:* I mention world space above because it is one of the coordinate systems in Scotty3D. But for all of the implementation details of this assignment, you'll be working in local space and/or the bone spaces. Indeed, it is not possible from within `Skeleton` to determine $L$, since `Skeleton`s don't know what instance they are being accessed through.


## `A4T2a` Forward Kinematics

**Implement:**
- `Skeleton::bind_pose`, which returns a vector whose $i\textrm{th}$ entry is $\hat{X}_{i\to\emptyset}$.
- `Skeleton::current_pose`, which returns a vector whose $i\textrm{th}$ entry is $X_{i\to\emptyset}$.

In the first half of this task, you will implement `Skeleton::bind_pose` and `Skeleton::current_pose`, which compute all the bind-to-local and pose-to-local transformations $B_j$ and $P_j$, respectively. It makes sense to compute these transformations in bulk because they are defined recursively in terms of a bone-to-parent transformation.

The transform between every bone's local space its parent is a rotation around a local $x$, $y$, and $z$ rotation axis (in that order) followed by a translation by the parent's `extent`.

In other words, given bone $b$ with rotation axis directions $x_b$, $y_b$, and $z_b$; local rotation amounts $rx_b$, $ry_b$, and $rz_b$; and parent bone $p$ with extent $e_p$, the transform that takes points in the local space of $b$ to the local space of $p$ is the matrix:

$$X_{p \gets b} \equiv T_{p_e} R_{rz_b @ z_b} R_{ry_b @ y_b} R_{rx_b @ x_b}$$

Where $T_{p_e}$ is a translation by $p_e$ and $R_{ra @ a}$ rotates by angle $ra$ around axis $a$.

Bones without a parent are just translated by the skeleton's base point, $r$, and current base offset, $o$:
$$X_{\emptyset \gets b} \equiv T_{r+o}$$

The overall transform from each bone to local space can be computed recursively:
$$X_{\emptyset \gets b} \equiv X_{\emptyset \gets p} X_{p \gets b}$$


In the _bind pose_ the bones are not posed or offset, so we have:

$$\hat{X}_{p \gets b} \equiv T_{p_e}$$
$$\hat{X}_{\emptyset \gets b} \equiv T_{r}$$
$$\hat{X}_{\emptyset \gets b} \equiv \hat{X}_{\emptyset \gets p} \hat{X}_{p \gets b}$$


Implementation notes:
 - The list of bones in the skeleton is guaranteed to be sorted such that bones' parent appear first, so your code should be able to work in a single pass through the bones array.
 - The functions `Mat4::angle_axis` and `Mat4::translate` will be helpful.

### Aside: Local Axes

Why do we bother specifying $x_b$, $y_b$, $z_b$?
Having a good choice of local axis is important when rigging characters for effective posing. If a joint has one primary axis of rotation, you'd prefer to have that axis aligned with (say) $rx_b$ so you need only adjust one variable when animating. This is especially important with mechanical assemblies, where you may well want a joint to represent a 1-DOF hinge, which is hard if the hinge is not parallel to some local axis of rotation!


The local rotation axes for a bone in Scotty3D are defined as follows (already implemented for you in the `Bone::compute_rotation_axes` function).

The $y_b$ axis is aligned with the bone's extent:
$$y_b \equiv \mathrm{normalize}(e_b)$$
(In case the bone's extent is zero, $y_b$ is set to $(0,1,0)$.)

To define $x_b$ we start with $\hat{x}$, the axis perpendicular to $y_b$ and aligned with the skeleton's $x$ axis:
$$\hat{x} \equiv \mathrm{normalize}((1,0,0) - ((1,0,0) \cdot y_b) y_b)$$
(In case $y_b$ is parallel to the $x$ axis, $\hat{x}$ is set to $(0,0,1)$.)

The $x_b$ axis is $\hat{x}$ rotated by `bone.roll` ($\theta_b$) around $y_b$:
$$x_b \equiv \cos(\theta_b) \hat{x} - \sin(\theta_b) (\hat{x} \times y_b)$$

And, finally, $z_b$ axis is perpendicular to $x_b$ and $y_b$ to form a right-handed coordinate system:
$$z_b \equiv x_b \times y_b$$

### A picture of what's going on

_Note: These diagrams are in 2D for visual clarity, but we will work with a 3D kinematic skeleton._

Just like a `Scene::Transform`, the `pose` (and `extent`) of a `Bone` transform all of its children.
In the diagram below, $c_0$ is the parent of $c_1$ and $c_1$ is the parent of $c_2$.
When a translation of $x_0$ and rotation of $\theta_0$ is applied to $c_0$, all of the descendants are affected by this transformation as well.
Then, $c_1$ is rotated by $\theta_1$ which affects itself and $c_2$. Finally, when rotation of $\theta_2$ is applied to $c_2$, it only affects itself because it has no children.

<center><img src="T2/forward_kinematic_diagram.jpg" style="height:480px"></center>


Once you have implemented these basic kinematics, you should be able to define skeletons, set their positions at a collection of keyframes, and watch the skeleton smoothly interpolate the motion (see the [user guide](https://cmu-graphics.github.io/Scotty3D-docs/guide/animate_mode/) for an explanation of the interface and some demo videos).

Note that the skeleton does not yet influence the geometry of the cube in this scene -- that will come in Task 3!


## `A4T2b` Inverse Kinematics

**Implement:**
- `Skeleton::gradient_in_current_pose`, which returns $\nabla f$.
- `Skeleton::solve_ik`, takes downhill steps in bone pose space to minimize $f$.

Now that we have a logical way to move joints around, we can implement Inverse Kinematics (IK), which will move the joints around in order to reach a target point. There are a few different ways we can do this, but for this assignment we'll implement an iterative method called gradient descent in order to find the minimum of a function. For a function $f : \mathbb{R}^n \to \mathbb{R}$, we'll have the update scheme:

$$q \gets q - \tau \nabla f$$

Where $\tau$ is a small timestep and $q$ holds all the `Bone::pose` *q*ordinates.
Specifically, we'll be using gradient descent to find the minimum of a cost function which measures the total squared distance between a set of IK handles positions $h$ and their associated bones $b$:

$$f(q) = \sum_{(h,i)} \frac{1}{2}|p_i(q) - h|^2 $$

Where $p_i(q) = X_{i \to \emptyset} [ e_i 1 ]^T $ is the end position of bone $i$.


*Implementation Notes:*
- If your updates become unstable, use a smaller $\tau$ (timestep).
- For even faster and better results, you can also implement a variable timestep instead of a fixed one. (One strategy: do a binary search to find a step which produces the large decrease in $f$ along $-\nabla f$.)
- Gradient descent should never affect `Skeleton::base_offset`.
- Remember what coordinate frame you're in. If you calculate the gradients in the wrong coordinate frame or use the axis of rotation in the wrong coordinate frame your answers will come out very wrong!
- Gradient descent is hard to get perfectly right because (especially with adaptive stepping!) it is a method that _really wants_ to work. Sometimes it's hard to distinguish an inefficient implementation from an incorrect one.

### Computing $\nabla f$

The gradient of $f$ (i.e., $\nabla f$) is the vector consisting of the partial derivatives of $f$ with respect to every element of $q$.
So let's look at one example -- let's take the partial derivative with respect to $ry_b$ -- the rotation around the local $y_b$ axis of bone $b$.

First, we expand the definition of $f$:
$$\frac{\partial}{\partial ry_b} f = \sum_{(h,i)} \frac{\partial}{\partial ry_b} \frac{1}{2}|p_i(\mathbf{\theta}) - h|^2 $$

In other words, we can compute the partial derivatives for each IK handle individually and sum them.
Any IK handle where $b$ isn't in the kinematic chain will contribute nothing to the partial derivative, so let's look at a handle whose bone is a descendant of $b$, and apply the chain rule:

$$\frac{\partial}{\partial ry_b} \frac{1}{2}|p_i(q) - h|^2
 = (p_i(q) - h) \cdot \frac{\partial}{\partial ry_b} p_i(q) $$

Then expand the definition of $p_i$:
$$= (p_i(q) - h) \cdot \frac{\partial}{\partial ry_b} X_{\emptyset \gets i} [ e_i 1 ]^T $$
$$= (p_i(q) - h) \cdot \frac{\partial}{\partial ry_b} X_{\emptyset \gets \ldots \gets c} X_{c \gets b} X_{b \gets \ldots \gets i} [ e_i 1 ]^T $$
$$= (p_i(q) - h) \cdot ( X_{outer} \frac{\partial}{\partial ry_b} R_{rx_b @ x_b} X_{inner} [ e_i 1 ]^T ) $$

In other words, you need only to project the end effector position to the appropriate space, compute the partial derivative of a rotation relative to its angle, and project this partial derivative to skeleton-local space.


### Some older words that need to be removed

Now we just need a way to calculate the Jacobian of $\theta$. For this, we can use the fact that:

$$(J_\theta)_i = \overrightarrow{\textbf{r}} \times \overrightarrow{\textbf{p}}$$

Where:

*   $J_i$ is the $i$th column of $J_\theta$.
*   $\overrightarrow{\textbf{r}}$ is the axis of rotation in the current joint space.
*   $\overrightarrow{\textbf{p}}$is the vector from the base of joint $i$ to the end point of the target joint.

For a more in-depth derivation of Jacobian transpose (and a look into other inverse kinematics algorithms), please check out [this presentation](https://web.archive.org/web/20190501035728/https://autorob.org/lectures/autorob_11_ik_jacobian.pdf). (Pages 45-56 in particular)


In order to implement this, you should update `Joint::compute_gradient` and `Skeleton::step_ik`. `Joint::compute_gradient` should calculate the gradient of $$\theta$$ in the rotated $$x$$, $$y$$, and $$z$$ directions, and add them to `Joint::angle_gradient` for all relevant joints. `Skeleton::step_ik` should actually do the gradient descent calculations and update the `pose` of each joint. In this function, you should use a small timestep, but do several iterations (say, $$10$$s to $$100$$s) of gradient descent in order to speed things up.


### Using your IK!
Once you have IK implemented, you should be able to create a series of joints, and get a particular joint to move to the desired final position you have selected.

Please refer to the [User Guide](https://cmu-graphics.github.io/Scotty3D-docs/guide/rigging_mode/) for more examples

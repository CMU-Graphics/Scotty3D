# `A4T1` Spline Interpolation

As we discussed in class, data points in time can be interpolated by constructing an approximating piecewise polynomial or spline. In this assignment you will implement a particular kind of spline, called a Catmull-Rom spline. A Catmull-Rom spline is a piecewise cubic spline defined purely in terms of the points it interpolates. It is a popular choice in animation systems, because the animator does not need to define additional data like tangents, etc. (However, your code may still need to numerically evaluate these tangents after the fact; more on this point later.) All of the methods relevant to spline interpolation can be found in `src/geometry/spline.h` with implementations in `src/geometry/spline.cpp`.

## `A4T1a` [Hermite Curve](https://en.wikipedia.org/wiki/Cubic_Hermite_spline) over the Unit Interval

Recall that a cubic polynomial is a function of the form:

$$p(t) = at^3 + bt^2 + ct + d$$

where $a$, $b$, $c$, and $d$ are fixed coefficients. However, there are many different ways of specifying a cubic polynomial. In particular, rather than specifying the coefficients directly, we can specify the endpoints and tangents we wish to interpolate. This construction is called the "Hermite form" of the polynomial. In particular, the Hermite form is given by

$$p(t) = h_{00}(t)p_{0} + h_{10}(t)m_{0} + h_{01}(t)p_{1} + h_{11}(t)m_{1}$$

where $p_0$, $p_1$ are endpoint positions, $m_0$, $m_1$ are endpoint tangents, and $h_{ij}$ are the Hermite bases

$$
\begin{align*}
h_{00}(t) &= 2t^3 - 3t^2 + 1 \\
h_{10}(t) &= t^3 - 2t^2 + t \\ 
h_{01}(t) &= -2t^3 + 3t^2 \\
h_{11}(t) &= t^3 - t^2
\end{align*}
$$

Your first task is to implement the method `Spline::cubic_unit_spline()`, which evaluates a spline defined over the time interval $[0, 1]$ given a pair of endpoints and tangents at endpoints.

Your basic strategy for implementing this routine should be:

*   Evaluate the time, its square, and its cube (for readability, you may want to make a local copy).
*   Using these values, as well as the position and tangent values, compute the four basis functions $h_{00}$, $h_{01}$, $h_{10}$, $h_{11}$ of a cubic polynomial in Hermite form.
*   Finally, combine the endpoint and tangent data using the evaluated bases, and return the result.

Notice that this function is templated on a type `T`. In C++, a templated class can operate on data of a variable type. In the case of a spline, for instance, we want to be able to interpolate all sorts of data: angles, vectors, colors, etc. So it wouldn't make sense to rewrite our spline class once for each of these types; instead, we use templates. In terms of implementation, your code will look no different than if you were operating on a basic type (e.g., doubles). However, the compiler will complain if you try to interpolate a type for which interpolation doesn't make sense! For instance, if you tried to interpolate `Skeleton` objects, the compiler would likely complain that there is no definition for the sum of two skeletons (via a `+` operator). In general, our spline interpolation will only make sense for data that comes from a vector space, since we need to add `T` values and take scalar multiples.

## `A4T1b` Evaluation of a Catmull-Rom spline

In general we will want smooth splines between a long sequence of points. You will now implement the method `Spline::at()` which evaluates a general Catmull-Rom spline at the specified time in a sequence of points (called "knots"). Since you already have code to interpolate a pair of endpoints and tangents (`cubic_unit_spline` from the previous part), the only task remaining is to find the interval closest to the query time, and evaluate its endpoints and tangents.

<p align="center">
    <img src="T1/Catmull-Rom.svg" style="height:600px">
</p>

The basic idea behind Catmull-Rom is that for a given time $t$, we first find the knots at times

$$t_0 < t_1 \leq t < t_2 < t_3$$

We then use $t_1$ and $t_2$ as the endpoints of our cubic "piece," and for tangents we use the values

$$m_0 = \frac{p_2 - p_0}{t_2 - t_0}$$

$$m_1 = \frac{p_3 - p_1}{t_3 - t_1}$$

In other words, a reasonable guess for the tangent is given by the difference between neighboring points. (See the [Wikipedia](https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Interpolation_on_an_arbitrary_interval) and our [course slides](http://15462.courses.cs.cmu.edu/spring2021/lecture/anim/slide_044) for more details.)

This scheme works great if we have two well-defined knots on either side of the query time $t$. But what happens if we get a query time near the beginning or end of the spline? Or what if the spline contains fewer than four knots? We still have to somehow come up with a reasonable definition for the positions and tangents of the curve at these times. For this assignment, your Catmull-Rom spline interpolation should satisfy the following properties:

* If there are no knots at all in the spline, interpolation should return the default value for the interpolated type. This value can be computed by simply calling the constructor for the type: `T()`. For instance, if the spline is interpolating `Vec3` objects, then the default value will be $(0, 0, 0)$.
* If there is only one knot in the spline, interpolation should always return the value of that knot (independent of the time). In other words, we simply have a constant interpolant.
* If the query time is less than or equal to the initial knot, return the initial knot's value.
* If the query time is greater than or equal to the final knot, return the final knot's value.

<p align="center">
    <img src="T1/Catmull-Rom_mirroring.svg" style="height:600px">
</p>

Once we have two or more knots, interpolation can be handled using general-purpose code. In particular, we can adopt the following "mirroring" strategy to obtain the four knots used in our computation:

* Any query time between the first and last knot will have at least one knot "to the left" ( $k_1$ ) and one "to the right" ( $k_2$ ).
* Suppose we don't have a knot "two to the left" ( $k_0$ ). Then we will define a "virtual" knot $k_0 \equiv k_1 - (k_2 - k_1)$. In other words, we will "mirror" the difference we observe between $k_1$ and $k_2$ to the other side of $k_1$. The knot values should be mirrored in the same way. This is illustrated in the figure above.
* Likewise, if we don't have a knot "two to the right" ( $k_3$ ), then we will "mirror" the difference to get a "virtual" knot ( $k_3 = k_2 + (k_2 - k_1)$ ).
* At this point, we have four valid knot values (whether "real" or "virtual"), and can compute our tangents and positions as usual.
* These values are then handed off to our subroutine that computes cubic interpolation over the unit interval.

<p align="center">
    <img src="T1/evaluate_catmull_rom_spline.png" style="height:300px">
</p>

An important thing to keep in mind is that `Spline::cubic_unit_spline()` assumes that the time value $t$ is between $0$ and $1$, whereas the distance between two knots on our Catmull-Rom spline can be arbitrary. Therefore, when calling this subroutine you will have to normalize $t$ such that it is between $0$ and $1$, i.e., you will have to divide by the length of the current interval over which you are interpolating. You should think very carefully about how this normalization affects the value computed by the subroutine, in comparison to the values we want to return. A transformation is necessary for the tangents that you feed in to specify the unit spline. Refer to the wikipedia or course slides for more information if you're stuck.

Internally, a `Spline` object stores its data in an standard library `map<float, T>` that maps knot times to knot values. A nice thing about a `map` is that it automatically keeps knots in sorted order. Therefore, we can quickly access a knot close to a given time using the method [`map::upper_bound`](https://en.cppreference.com/w/cpp/container/map/upper_bound), which returns an iterator to the knot with the smallest time greater than the given query time.

## Using the splines

Once you have implemented the functions in `spline.cpp`, you should be able to make simple animations by translating, rotating or scaling the mesh in the scene. The main idea is to:
* create an initial keyframe by clicking at a point on the white timeline at the bottom of the screen
* specify the initial location/orientation/scale of your mesh using the controls provided
* create more keyframes with different mesh locations/orientations/scales and watch the splines smoothly interpolate the movement of your mesh!


Animating a cow:

https://user-images.githubusercontent.com/1418494/202866181-01bf6965-b247-4a8a-b23a-ae9340d9c6ac.mp4

([local copy of this video](T1/cow-animate.mp4) )

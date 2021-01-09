
#pragma once

#include "../lib/mathlib.h"

class Camera {
public:
    Camera(Vec2 dim);

    /**
            Returns a world-space ray from the camera that corresponds to a
            ray exiting the camera that deposits light at the sensor plane
            position given by (x,y). x and y are provided in the normalized
            coordinate space of the sensor. For example (0.5, 0.5)
            corresponds to the middle of the screen.
    */
    Ray generate_ray(Vec2 screen_coord) const;

    /// View transformation matrix
    Mat4 get_view() const;
    /// Perspective projection transformation matrix
    Mat4 get_proj() const;

    /// Camera position
    Vec3 pos() const;
    /// Camera look position
    Vec3 center() const;
    /// Camera look direction
    Vec3 front() const;

    /// Get distance from the current position to the viewpoint
    float dist() const;

    /// Set camera at a position and a center to look at
    void look_at(Vec3 cent, Vec3 pos);

    /// Reset to default values
    void reset();

    /// Apply movement delta to orbit position
    void mouse_orbit(Vec2 off);
    /// Apply movement delta to look point
    void mouse_move(Vec2 off);
    /// Apply movement delta to radius (distance from look point)
    void mouse_radius(float off);

    /// Unecessary helpers
    void set_ar(float ar);
    void set_ar(Vec2 dim);
    float get_ar() const;
    void set_ap(float ap);
    float get_ap() const;
    void set_dist(float dist);
    float get_dist() const;
    void set_fov(float fov);
    float get_fov() const;
    float get_h_fov() const;
    float get_near() const;

private:
    void update_pos();

    /// Camera parameters
    Vec3 position, looking_at;
    /// FOV is in degrees
    float vert_fov, aspect_ratio;
    /// Current camera rotation
    Quat rot;

    /// For updating position & looking_at
    float radius, near_plane;
    /// For mouse control
    float orbit_sens, move_sens, radius_sens;
    /// Lens parameters
    float aperture, focal_dist;

    /// Cached view matrices
    Mat4 view, iview;
};

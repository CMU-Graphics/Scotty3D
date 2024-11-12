% We construct a simple example of 2 bones b and i, where b is i's parent.

e = [1; 2; 3]; % the local coordinate of a point on bone i for matching the target position
a = 0.1; % bone b's rotation angle around y axis 
b = 0.2; % bone b's rotation angle around z axis
c = 0.3; % bone b's rotation angle around x axis

Xbi = [1 0 0 2; 0 1 0 0; 0 0 1 0; 0 0 0 1]; % the transformation from bone i's local coordinate to bone b's local coordinate
Rx = [1 0 0 0; 0 cos(c) -sin(c) 0; 0 sin(c) cos(c) 0; 0 0 0 1]; % bone b's rotation matrix around x axis
Ry = [cos(a) 0 sin(a) 0; 0 1 0 0; -sin(a) 0 cos(a) 0; 0 0 0 1]; % bone b's rotation matrix around y axis
Rz = [cos(b) -sin(b) 0 0; sin(b) cos(b) 0 0; 0 0 1 0; 0 0 0 1]; % bone b's rotation matrix around z axis
T = [1 0 0 3; 0 1 0 1; 0 0 1 0; 0 0 0 1]; % the translation from bone b's local coordinate to skeleton local coordinate 

p = T * Rz * Ry * Rx * Xbi * [e; 1]; % skeleton local coordinate of the point

% derivative computated by differentiating the rotation matrix:
dpda1 = T * Rz * [-sin(a) 0 cos(a) 0; 0 0 0 0; -cos(a) 0 -sin(a) 0; 0 0 0 0] * Rx * Xbi * [e; 1]

% derivative computated by the geometric intuition:
x = T * Rz * Ry * [0; 1; 0; 0];
x = x(1:3);
r = T * Rz * Ry * Rx * [0; 0; 0; 1];
r = r(1:3);
dpda2 = cross(x, p(1:3) - r)

#include "scene.h"

Collision Wall::intersect(ray r)
{

    // check determinant so we do not attempt to solve an unsolvable system (otherwise divide by zero will occur)
    double det = (r.direction.x * (-direction.y) - (-direction.x * r.direction.y));
    if (det == 0)
        return Collision(-1, vec2(0, 0), false);

    vec2 rhs = r.origin * (-1.) + origin;

    double first_step_fraction = r.direction.y / r.direction.x;

    double p2 = (rhs.y - first_step_fraction * rhs.x) / (-direction.y + first_step_fraction * direction.x);

    double p1 = (rhs.x + p2 * direction.x) / r.direction.x;

    bool hit = p1 > 0 && p2 >= 0 && p2 <= upper_bound;

    vec2 normal(direction.y, -direction.x);
    if (normal.dot(r.direction) > 0)
        normal = normal * (-1.);

    return Collision(r.direction.length() * p1, normal, hit);
}

Collision Circle::intersect(ray r)
{
    vec2 z = r.origin + center * (-1);

    double a = r.direction.x * r.direction.x + r.direction.y * r.direction.y;

    double b = 2 * (r.direction.x * z.x + r.direction.y * z.y);

    double c = z.dot(z) - radius * radius;

    double det = b * b - 4 * a * c;

    double p = -1;

    if (det < 0)
        return Collision(-1, vec2(0, 0), false);
    vec2 intersection_point(0, 0);
    if (det == 0)
        intersection_point = r.origin + r.direction * (-b / (2 * a));
    else
    {
        double p1 = (-b + sqrt(det)) / (2 * a);
        double p2 = (-b - sqrt(det)) / (2 * a);

        p = p1 < p2 ? p1 : p2;
        intersection_point = r.origin + r.direction * p;
    }

    return Collision(p * r.direction.length(), intersection_point + center * (-1), p > 0);
}
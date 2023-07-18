#include "Collision.h"

#include <cassert>

namespace {
    Vector3 Support(const Collider& collider1, const Collider& collider2, const Vector3& direction) {
        return collider1.FindFurthestPoint(direction) - collider2.FindFurthestPoint(-direction);
    }

    bool SameDirection(const Vector3& a, const Vector3& b) {
        return Dot(a, b) > 0;
    }

    bool LineCase(std::vector<Vector3>& simplex, Vector3& direction) {
        assert(simplex.size() == 2);
        Vector3 a = simplex[0];
        Vector3 b = simplex[1];

        Vector3 ab = b - a;
        Vector3 ao = -a;
        direction = Cross(Cross(ab, ao), ab);
        return false;
    }

    bool TriangleCase(std::vector<Vector3>& simplex, Vector3& direction) {
        assert(simplex.size() == 3);
        Vector3 a = simplex[0];
        Vector3 b = simplex[1];
        Vector3 c = simplex[2];

        Vector3 ab = b - a;
        Vector3 ac = c - a;
        Vector3 ao = -a;

        Vector3 abc = Cross(ab, ac);

        if (SameDirection(Cross(abc, ac), ao)) {
            if (SameDirection(ac, ao)) {
                simplex = { a,c };
                direction = Cross(Cross(ac, ao), ac);
            }
            else {
                return LineCase(simplex = { a,b }, direction);
            }
        }
        else {
            if (SameDirection(Cross(ab, abc), ao)) {
                return LineCase(simplex = { a,b }, direction);
            }
            else {
                if (SameDirection(abc, ao)) {
                    direction = abc;
                }
                else {
                    simplex = { a,c,b };
                    direction - abc;
                }
            }
        }
        return false;
    }

    bool Tetrahedron(std::vector<Vector3>& simplex, Vector3& direction) {
        assert(simplex.size() == 4);
        Vector3 a = simplex[0];
        Vector3 b = simplex[1];
        Vector3 c = simplex[2];
        Vector3 d = simplex[3];

        Vector3 ab = b - a;
        Vector3 ac = c - a;
        Vector3 ad = d - a;
        Vector3 ao = -a;

        Vector3 abc = Cross(ab, ac);
        Vector3 acd = Cross(ac, ad);
        Vector3 adb = Cross(ad, ab);

        if (SameDirection(abc, ao)) {
            return TriangleCase(simplex = { a,b,c }, direction);
        }
        if (SameDirection(acd, ao)) {
            return TriangleCase(simplex = { a,c,d }, direction);
        }
        if (SameDirection(adb, ao)) {
            return TriangleCase(simplex = { a,d,b }, direction);
        }
        return true;
    }

    bool NextSimplex(std::vector<Vector3>& simplex, Vector3& direction) {
        switch (simplex.size()) {
        case 2: LineCase(simplex, direction);
        case 3: TriangleCase(simplex, direction);
        case 4: Tetrahedron(simplex, direction);
        }
        return false;
    }

}

bool GJK(const Collider& collider1, const Collider& collider2, GJKInfo* gjkInfo) {
    Vector3 direction = Vector3::unitX;
    Vector3 support = Support(collider1, collider2, direction);

    std::vector<Vector3> simplex;
    simplex.emplace_back(support);

    direction = -support;

    while (true) {
        assert(simplex.size() < 5);

        support = Support(collider1, collider2, direction);

        if (Dot(support, direction) <= 0) {
            return false;
        }

        simplex.emplace_back(support);

        if (NextSimplex(simplex, direction)) {
            break;
        }
    }
    gjkInfo->simplex = std::move(simplex);
    return true;
}

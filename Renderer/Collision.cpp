#include "Collision.h"

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace {

    struct Simplex {
        Simplex& operator=(std::initializer_list<Vector3> list) {
            for (auto iter = list.begin(); iter != list.end(); ++iter) {
                points_[std::distance(list.begin(), iter)] = *iter;
            }
            count_ = uint32_t(list.size());
            return *this;
        }
        Vector3& operator[](size_t i) { return points_[i]; }
        size_t Count() const { return count_; }
        void Push(const Vector3& p) {
            points_ = { p, points_[0], points_[1], points_[2] };
            count_ = std::min(count_ + 1, 4u);
        }
        operator const std::array<Vector3, 4>& () const { return points_; }
    private:
        std::array<Vector3, 4> points_{};
        uint32_t count_{ 0 };
    };

    Vector3 Support(const Collider& collider1, const Collider& collider2, const Vector3& direction) {
        return collider1.FindFurthestPoint(direction) - collider2.FindFurthestPoint(-direction);
    }

    void TriangleCase(Simplex& simplex, Vector3& direction) {
        Vector3 a = simplex[0];
        Vector3 b = simplex[1];
        Vector3 c = simplex[2];

        Vector3 ab = b - a;
        Vector3 ac = c - a;
        Vector3 ao = -a;

        Vector3 normal = Cross(ab, ac);
        if (Dot(Cross(ab, normal), ao) > 0.0f) {
            simplex = { b, a };
            direction = Cross(Cross(ab, ao), ab);
            return;
        }
        if (Dot(Cross(normal, ac), ao) > 0.0f) {
            simplex = { a, c };
            direction = Cross(Cross(ac, ao), ac);
            return;
        }

        if (Dot(normal, ao) > 0.0f) {
            direction = normal;
            return;
        }
        simplex = { a, c, b };
        direction = -normal;
        return;
    }

    bool TetrahedronCase(Simplex& simplex, Vector3& direction) {
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

        if (Dot(abc, ao) > 0.0f) {
            direction = abc;
            return false;
        }
        if (Dot(acd, ao) > 0.0f) {
            simplex = { a, c, d };
            direction = acd;
            return false;
        }
        if (Dot(adb, ao) > 0.0f) {
            simplex = { a, d, b };
            return false;
        }
        return true;
    }

}

bool GJK(const Collider& collider1, const Collider& collider2, GJKInfo* gjkInfo) {
    constexpr uint32_t kMaxIterationCount = 64;
    Simplex simplex;
    Vector3 direction, support;

    direction = collider2.transform.GetWorldPosition() - collider1.transform.GetWorldPosition();
    support = Support(collider1, collider2, direction);
    simplex.Push(support);

    direction = -support;
    support = Support(collider1, collider2, direction);
    if (Dot(support, direction) < 0) { return false; }
    simplex.Push(support);

    {
        Vector3 ab = simplex[1] - simplex[0];
        Vector3 ao = -simplex[0];
        direction = Cross(Cross(ab, ao), ab);
        if (direction == Vector3::zero) {
            direction = Cross(ab, Vector3::unitX);
            if (direction == Vector3::zero) {
                direction = Cross(ab, Vector3::unitZ);
            }
        }
    }

    for (uint32_t iterationCount = 0; iterationCount < kMaxIterationCount; ++iterationCount) {
        support = Support(collider1, collider2, direction);
        if (Dot(support, direction) < 0.0f) { return false; }
        simplex.Push(support);

        if (simplex.Count() == 3) {
            TriangleCase(simplex, direction);
        }
        else if (TetrahedronCase(simplex, direction)) {
            if (gjkInfo) {
                gjkInfo->simplex = simplex;
            }
            return true;
        }

    }
    return false;
}

EPAInfo EPA(const GJKInfo& gjkInfo, const Collider& collider1, const Collider& collider2) {
    constexpr uint32_t kMaxIterationCount = 64;
    constexpr float kDistanceTolerance = 0.0001f;
    std::vector<Vector3> vertices(gjkInfo.simplex.begin(), gjkInfo.simplex.end());

    using Edge = std::pair<uint32_t, uint32_t>;

    struct Face {
        std::array<uint32_t,3> index;
        Vector3 normal;
        float distance;
    };

    std::vector<Face> faces;
    auto AddFace = [&](uint32_t a, uint32_t b, uint32_t c) {
        auto& face = faces.emplace_back();
        face.index[0] = a, face.index[1] = b, face.index[2] = c;
        face.normal = Cross(vertices[b] - vertices[a], vertices[c] - vertices[a]).Normalized();
        face.distance = Dot(vertices[a], face.normal);
    };
    auto FaceDistance = [&](size_t faceIndex) {
        return Dot(vertices[faces[faceIndex].index[0]], faces[faceIndex].normal);
    };
    AddFace(0, 1, 2);
    AddFace(0, 2, 3);
    AddFace(0, 3, 1);
    AddFace(1, 3, 2);

    uint32_t closestFaceIndex = 0;

    for (uint32_t iterationCount = 0; iterationCount < kMaxIterationCount; ++iterationCount) {
        // 最も近い面を見つける
        float minDistance = FaceDistance(0);
        closestFaceIndex = 0;
        for (uint32_t i = 1; i < faces.size(); ++i) {
            float distance = FaceDistance(i);
            if (distance < minDistance) {
                minDistance = distance;
                closestFaceIndex = i;
            }
        }

        Vector3 direction = faces[closestFaceIndex].normal;
        Vector3 support = Support(collider1, collider2, direction);

        if (Dot(support, direction) - minDistance < kDistanceTolerance) {
            EPAInfo info;
            info.normal = faces[closestFaceIndex].normal;
            info.depth = Dot(support, direction);
            if (isnan(info.depth)) {
                return {};
            }
            return info;
        }

        vertices.emplace_back(support);

        std::vector<Edge> loseEdges;
        for (uint32_t i = 0; i < faces.size(); ++i) {
            if (Dot(faces[i].normal, support - vertices[faces[i].index[0]]) > 0) {
                for (uint32_t j = 0; j < 3; ++j) {
                    Edge currentEdge = { faces[i].index[j], faces[i].index[(j + 1) % 3] };
                    bool foundEdge = false;
                    for (uint32_t k = 0; k < loseEdges.size(); ++k) {
                        if (loseEdges[k].second == currentEdge.first && loseEdges[k].first == currentEdge.second) {
                            loseEdges[k] = loseEdges.back();
                            loseEdges.pop_back();
                            foundEdge = true;
                            k = uint32_t(loseEdges.size());
                        }
                    }

                    if (!foundEdge) {
                        loseEdges.emplace_back(currentEdge);
                    }
                }

                faces[i] = faces.back();
                faces.pop_back();
                --i;
            }
        }

        for (uint32_t i = 0; i < loseEdges.size(); ++i) {
            AddFace(loseEdges[i].first, loseEdges[i].second, uint32_t(vertices.size() - 1));
            auto& addedFace = faces.back();

            float bias = 0.000001f;
            if (Dot(vertices[addedFace.index[0]], addedFace.normal) + bias < 0) {
                uint32_t temp = addedFace.index[0];
                addedFace.index[0] = addedFace.index[1];
                addedFace.index[1] = temp;
                addedFace.normal = -addedFace.normal;
            }
        }
    }

    EPAInfo info;
    info.normal = faces[closestFaceIndex].normal;
    info.depth = Dot(vertices[faces[closestFaceIndex].index[0]], faces[closestFaceIndex].normal);
    if (isnan(info.depth)) {
        return {};
    }
    return info;
}

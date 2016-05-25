// Copyright (c) 2016 Doyub Kim
//
// Jet uses portion of pbrt-v2

// Copyright (c) 1998-2014, Matt Pharr and Greg Humphreys.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <pch.h>
#include <jet/triangle_mesh3.h>
#include <jet/parallel.h>

#include <obj/obj_parser.hpp>

#include <algorithm>
#include <limits>
#include <string>
#include <utility>  // just make cpplint happy..

using namespace jet;

inline std::ostream& operator<<(std::ostream& strm, const Vector2D& v) {
    strm << v.x << ' ' << v.y;
    return strm;
}

inline std::ostream& operator<<(std::ostream& strm, const Vector3D& v) {
    strm << v.x << ' ' << v.y << ' ' << v.z;
    return strm;
}

TriangleMesh3::TriangleMesh3() {
}

TriangleMesh3::TriangleMesh3(const TriangleMesh3& other) {
    set(other);
}

Vector3D TriangleMesh3::closestPoint(const Vector3D& otherPoint) const {
    static const double m = std::numeric_limits<double>::max();
    Vector3D minDistPt(m, m, m);
    double minDistSquared = m;

    size_t n = numberOfFaces();
    for (size_t i = 0; i < n; ++i) {
        Triangle3 tri = triangle(i);
        Vector3D pt = tri.closestPoint(otherPoint);
        double distSquared = (otherPoint - pt).lengthSquared();
        if (distSquared < minDistSquared) {
            minDistSquared = distSquared;
            minDistPt = pt;
        }
    }

    return minDistPt;
}

Vector3D TriangleMesh3::actualClosestNormal(const Vector3D& otherPoint) const {
    static const double m = std::numeric_limits<double>::max();
    Vector3D minDistPt(m, m, m);
    Vector3D minDistNormal(1, 0, 0);
    double minDistSquared = m;

    size_t n = numberOfFaces();
    for (size_t i = 0; i < n; ++i) {
        Triangle3 tri = triangle(i);
        Vector3D pt = tri.closestPoint(otherPoint);
        double distSquared = (otherPoint - pt).lengthSquared();
        if (distSquared < minDistSquared) {
            minDistSquared = distSquared;
            minDistPt = pt;
            minDistNormal = tri.closestNormal(otherPoint);
        }
    }

    return minDistNormal;
}

void TriangleMesh3::getClosestIntersection(
    const Ray3D& ray,
    SurfaceRayIntersection3* intersection) const {
    size_t n = numberOfFaces();
    double t = std::numeric_limits<double>::max();
    for (size_t i = 0; i < n; ++i) {
        Triangle3 tri = triangle(i);
        SurfaceRayIntersection3 tmpIntersection;
        tri.getClosestIntersection(ray, &tmpIntersection);
        if (tmpIntersection.t < t) {
            t = tmpIntersection.t;
            *intersection = tmpIntersection;
        }
    }
}

BoundingBox3D TriangleMesh3::boundingBox() const {
    BoundingBox3D box;
    size_t n = _pointIndices.size();
    for (size_t i = 0; i < n; ++i) {
        const Point3UI& face = _pointIndices[i];
        box.merge(_points[face[0]]);
        box.merge(_points[face[1]]);
        box.merge(_points[face[2]]);
    }
    return box;
}

bool TriangleMesh3::intersects(const Ray3D& ray) const {
    size_t n = numberOfFaces();
    for (size_t i = 0; i < n; ++i) {
        Triangle3 tri = triangle(i);
        if (tri.intersects(ray)) {
            return true;
        }
    }
    return false;
}

double TriangleMesh3::closestDistance(const Vector3D& otherPoint) const {
    double minDist = std::numeric_limits<double>::max();

    size_t n = numberOfFaces();
    for (size_t i = 0; i < n; ++i) {
        Triangle3 tri = triangle(i);
        minDist = std::min(minDist, tri.closestDistance(otherPoint));
    }

    return minDist;
}

void TriangleMesh3::clear() {
    _points.clear();
    _normals.clear();
    _uvs.clear();
    _pointIndices.clear();
    _normalIndices.clear();
    _uvIndices.clear();
    _areaCache.clear();
}

void TriangleMesh3::set(const TriangleMesh3& other) {
    _points.set(other._points);
    _normals.set(other._normals);
    _uvs.set(other._uvs);
    _pointIndices.set(other._pointIndices);
    _normalIndices.set(other._normalIndices);
    _uvIndices.set(other._uvIndices);
    _areaCache.set(other._areaCache);
}

void TriangleMesh3::swap(TriangleMesh3& other) {
    _points.swap(other._points);
    _normals.swap(other._normals);
    _uvs.swap(other._uvs);
    _pointIndices.swap(other._pointIndices);
    _normalIndices.swap(other._normalIndices);
    _uvIndices.swap(other._uvIndices);
    _areaCache.swap(other._areaCache);
}

double TriangleMesh3::area() const {
    double a = 0;
    for (size_t i = 0; i < numberOfFaces(); ++i) {
        Triangle3 tri = triangle(i);
        a += tri.area();
    }
    return a;
}

double TriangleMesh3::volume() const {
    double vol = 0;
    for (size_t i = 0; i < numberOfFaces(); ++i) {
        Triangle3 tri = triangle(i);
        vol += tri.points[0].dot(tri.points[1].cross(tri.points[2])) / 6.f;
    }
    return vol;
}

void TriangleMesh3::sample(
    double u1,
    double u2,
    double u3,
    Vector3D* x,
    Vector3D* n) const {
    if (_areaCache.size() != numberOfFaces() + 1) {
        computeAreaCacheP();
    }

    // pick a triangle
    const double* ptr = std::lower_bound(
        _areaCache.data(),
        _areaCache.data() + _areaCache.size(),
        u1);

    size_t offset = 0;
    if (ptr > _areaCache.data()) {
        offset = static_cast<size_t>(ptr - _areaCache.data() - 1);
    }

    // sample on the triangle
    Triangle3 tri = triangle(offset);
    tri.sample(u2, u3, x, n);
}

const Vector3D& TriangleMesh3::point(size_t i) const {
    return _points[i];
}

Vector3D& TriangleMesh3::point(size_t i) {
    return _points[i];
}

const Vector3D& TriangleMesh3::normal(size_t i) const {
    return _normals[i];
}

Vector3D& TriangleMesh3::normal(size_t i) {
    return _normals[i];
}

const Vector2D& TriangleMesh3::uv(size_t i) const {
    return _uvs[i];
}

Vector2D& TriangleMesh3::uv(size_t i) {
    return _uvs[i];
}

const Point3UI& TriangleMesh3::pointIndex(size_t i) const {
    return _pointIndices[i];
}

Point3UI& TriangleMesh3::pointIndex(size_t i) {
    return _pointIndices[i];
}

const Point3UI& TriangleMesh3::normalIndex(size_t i) const {
    return _normalIndices[i];
}

Point3UI& TriangleMesh3::normalIndex(size_t i) {
    return _normalIndices[i];
}

const Point3UI& TriangleMesh3::uvIndex(size_t i) const {
    return _uvIndices[i];
}

Point3UI& TriangleMesh3::uvIndex(size_t i) {
    return _uvIndices[i];
}

Triangle3 TriangleMesh3::triangle(size_t i) const {
    Triangle3 tri;
    for (int j = 0; j < 3; j++) {
        tri.points[j] = _points[_pointIndices[i][j]];
        if (hasUvs()) {
            tri.uvs[j] = _uvs[_uvIndices[i][j]];
        }
    }

    Vector3D n = tri.faceNormal();

    for (int j = 0; j < 3; j++) {
        if (hasNormals()) {
            tri.normals[j] = _normals[_normalIndices[i][j]];
        } else {
            tri.normals[j] = n;
        }
    }

    return tri;
}

size_t TriangleMesh3::numberOfPoints() const {
    return _points.size();
}

size_t TriangleMesh3::numberOfNormals() const {
    return _normals.size();
}

size_t TriangleMesh3::numberOfUvs() const {
    return _uvs.size();
}

size_t TriangleMesh3::numberOfFaces() const {
    return _pointIndices.size();
}

bool TriangleMesh3::hasNormals() const {
    return _normals.size() > 0;
}

bool TriangleMesh3::hasUvs() const {
    return _uvs.size() > 0;
}

void TriangleMesh3::addPoint(const Vector3D& pt) {
    _points.append(pt);
}

void TriangleMesh3::addNormal(const Vector3D& n) {
    _normals.append(n);
}

void TriangleMesh3::addUv(const Vector2D& t) {
    _uvs.append(t);
}

void TriangleMesh3::addPointFace(const Point3UI& newPointIndices) {
    _pointIndices.append(newPointIndices);
}

void TriangleMesh3::addPointNormalFace(
    const Point3UI& newPointIndices,
    const Point3UI& newNormalIndices) {
    // Number of normal indicies must match with number of point indices once
    // you decided to add normal indicies. Same for the uvs as well.
    JET_ASSERT(_pointIndices.size() == _normalIndices.size());

    _pointIndices.append(newPointIndices);
    _normalIndices.append(newNormalIndices);
}

void TriangleMesh3::addPointNormalUvFace(
    const Point3UI& newPointIndices,
    const Point3UI& newNormalIndices,
    const Point3UI& newUvIndices) {
    // Number of normal indicies must match with number of point indices once
    // you decided to add normal indicies. Same for the uvs as well.
    JET_ASSERT(_pointIndices.size() == _normalIndices.size());
    JET_ASSERT(_pointIndices.size() == _uvs.size());
    _pointIndices.append(newPointIndices);
    _normalIndices.append(newNormalIndices);
    _uvIndices.append(newUvIndices);
}

void TriangleMesh3::addPointUvFace(
    const Point3UI& newPointIndices,
    const Point3UI& newUvIndices) {
    // Number of normal indicies must match with number of point indices once
    // you decided to add normal indicies. Same for the uvs as well.
    JET_ASSERT(_pointIndices.size() == _uvs.size());
    _pointIndices.append(newPointIndices);
    _uvIndices.append(newUvIndices);
}

void TriangleMesh3::addTriangle(const Triangle3& tri) {
    size_t vStart = _points.size();
    size_t nStart = _normals.size();
    size_t tStart = _uvs.size();
    Point3UI newPointIndices;
    Point3UI newNormalIndices;
    Point3UI newUvIndices;
    for (size_t i = 0; i < 3; i++) {
        _points.append(tri.points[i]);
        _normals.append(tri.normals[i]);
        _uvs.append(tri.uvs[i]);
        newPointIndices[i] = vStart + i;
        newNormalIndices[i] = nStart + i;
        newUvIndices[i] = tStart + i;
    }
    _pointIndices.append(newPointIndices);
    _normalIndices.append(newNormalIndices);
    _uvIndices.append(newUvIndices);
}

void TriangleMesh3::setFaceNormal() {
    _normals.resize(_points.size());
    _normalIndices.set(_pointIndices);

    for (size_t i = 0; i < numberOfFaces(); ++i) {
        Triangle3 tri = triangle(i);
        Vector3D n = tri.faceNormal();
        Point3UI f = _pointIndices[i];
        _normals[f.x] = n;
        _normals[f.y] = n;
        _normals[f.z] = n;
    }
}

void TriangleMesh3::setAngleWeightedVertexNormal() {
    _normals.clear();
    _normalIndices.clear();

    Array1<double> angleWeights(_points.size());
    Vector3DArray pseudoNormals(_points.size());

    for (size_t i = 0; i < _points.size(); ++i) {
        angleWeights[i] = 0;
        pseudoNormals[i] = Vector3D();
    }

    for (size_t i = 0; i < numberOfFaces(); ++i) {
        Vector3D pts[3];
        Vector3D normal, e0, e1;
        double cosangle, angle;
        size_t idx[3];

        // Quick references
        for (int j = 0; j < 3; j++) {
            idx[j] = _pointIndices[i][j];
            pts[j] = _points[idx[j]];
        }

        // Angle for point 0
        e0 = pts[1] - pts[0];
        e1 = pts[2] - pts[0];
        e0.normalize();
        e1.normalize();
        normal = e0.cross(e1);
        normal.normalize();
        cosangle = clamp(e0.dot(e1), -1.0, 1.0);
        angle = std::acos(cosangle);
        angleWeights[idx[0]] += angle;
        pseudoNormals[idx[0]] += angle*normal;

        // Angle for point 1
        e0 = pts[2] - pts[1];
        e1 = pts[0] - pts[1];
        e0.normalize();
        e1.normalize();
        normal = e0.cross(e1);
        normal.normalize();
        cosangle = clamp(e0.dot(e1), -1.0, 1.0);
        angle = std::acos(cosangle);
        angleWeights[idx[1]] += angle;
        pseudoNormals[idx[1]] += angle*normal;

        // Angle for point 2
        e0 = pts[0] - pts[2];
        e1 = pts[1] - pts[2];
        e0.normalize();
        e1.normalize();
        normal = e0.cross(e1);
        normal.normalize();
        cosangle = clamp(e0.dot(e1), -1.0, 1.0);
        angle = std::acos(cosangle);
        angleWeights[idx[2]] += angle;
        pseudoNormals[idx[2]] += angle*normal;
    }

    for (size_t i = 0; i < _points.size(); ++i) {
        if (angleWeights[i] > 0) {
            pseudoNormals[i] /= angleWeights[i];
        }
    }

    std::swap(pseudoNormals, _normals);
    _normalIndices.set(_pointIndices);
}

void TriangleMesh3::clearAreaCache() {
    _areaCache.clear();
}

void TriangleMesh3::computeAreaCache() {
    _areaCache.resize(numberOfFaces()+1);

    // compute area weighted CDF
    _areaCache[0] = 0;
    for (size_t i = 1; i < _areaCache.size(); ++i) {
        Triangle3 tri = triangle(i-1);
        double triarea = tri.area();
        _areaCache[i] = triarea + _areaCache[i-1];
    }

    // normalize areaCache
    for (size_t i = 1; i < _areaCache.size(); ++i) {
        _areaCache[i] /= _areaCache[numberOfFaces()];
    }
}

void TriangleMesh3::computeAreaCacheP() const {
    _areaCache.resize(numberOfFaces()+1);

    // compute area weighted CDF
    _areaCache[0] = 0;
    for (size_t i = 1; i < _areaCache.size(); ++i) {
        Triangle3 tri = triangle(i-1);
        double triarea = tri.area();
        _areaCache[i] = triarea + _areaCache[i-1];
    }

    // normalize areaCache
    for (size_t i = 1; i < _areaCache.size(); ++i) {
        _areaCache[i] /= _areaCache[numberOfFaces()];
    }
}

void TriangleMesh3::scale(double factor) {
    parallelFor(
        kZeroSize,
        numberOfPoints(),
        [this, factor](size_t i) {
            _points[i] *= factor;
        });
}

void TriangleMesh3::translate(const Vector3D& t) {
    parallelFor(
        kZeroSize,
        numberOfPoints(),
        [this, t](size_t i) {
            _points[i] += t;
        });
}

void TriangleMesh3::rotate(const Quaternion<double>& q) {
    parallelFor(
        kZeroSize,
        numberOfPoints(),
        [this, q](size_t i) {
            _points[i] = q * _points[i];
        });

    parallelFor(
        kZeroSize,
        numberOfNormals(),
        [this, q](size_t i) {
            _normals[i] = q * _normals[i];
        });
}

void TriangleMesh3::writeObj(std::ostream* strm) const {
    // vertex
    for (const auto& pt : _points) {
        (*strm) << "v " << pt << std::endl;
    }

    // uv coords
    for (const auto& uv : _uvs) {
        (*strm) << "vt " << uv << std::endl;
    }

    // normals
    for (const auto& n : _normals) {
        (*strm) << "vn " << n << std::endl;
    }

    // faces
    bool hasUvs_ = hasUvs();
    bool hasNormals_ = hasNormals();
    for (size_t i = 0; i < numberOfFaces(); ++i) {
        (*strm) << "f ";
        for (int j = 0; j < 3; ++j) {
            (*strm) << _pointIndices[i][j] + 1;
            if (hasNormals_ || hasUvs_) {
                (*strm) << '/';
            }
            if (hasUvs_) {
                (*strm) << _uvIndices[i][j] + 1;
            }
            if (hasNormals_) {
                (*strm) << '/' << _normalIndices[i][j] + 1;
            }
            (*strm) << ' ';
        }
        (*strm) << std::endl;
    }
}

bool TriangleMesh3::readObj(std::istream* strm) {
    obj::obj_parser parser(
        obj::obj_parser::triangulate_faces
        | obj::obj_parser::translate_negative_indices);

    parser.info_callback(
        [](size_t lineNumber, const std::string& message){
            std::cout << lineNumber << " " << message << std::endl;
        });
    parser.warning_callback(
        [](size_t lineNumber, const std::string& message){
            std::cerr << lineNumber << " " << message << std::endl;
        });
    parser.error_callback([](size_t lineNumber, const std::string& message){
            std::cerr << lineNumber << " " << message << std::endl;
        });

    parser.geometric_vertex_callback(
        [this](obj::float_type x, obj::float_type y, obj::float_type z) {
            addPoint({x, y, z});
        });

    parser.texture_vertex_callback(
        [this](obj::float_type u, obj::float_type v) {
            addUv({u, v});
        });

    parser.vertex_normal_callback(
        [this](obj::float_type nx, obj::float_type ny, obj::float_type nz) {
            addNormal({nx, ny, nz});
        });

    parser.face_callbacks(
        // triangular_face_geometric_vertices_callback_type
        [this](obj::index_type v0, obj::index_type v1, obj::index_type v2) {
            addPointFace({v0 - 1, v1 - 1, v2 - 1});
        },
        // triangular_face_geometric_vertices_texture_vertices_callback_type
        [this](const obj::index_2_tuple_type& v0_vt0,
           const obj::index_2_tuple_type& v1_vt1,
           const obj::index_2_tuple_type& v2_vt2) {
            addPointUvFace(
                {
                    std::get<0>(v0_vt0) - 1,
                    std::get<0>(v1_vt1) - 1,
                    std::get<0>(v2_vt2) - 1
                },
                {
                    std::get<1>(v0_vt0) - 1,
                    std::get<1>(v1_vt1) - 1,
                    std::get<1>(v2_vt2) - 1
                });
        },
        // triangular_face_geometric_vertices_vertex_normals_callback_type
        [this](const obj::index_2_tuple_type& v0_vn0,
           const obj::index_2_tuple_type& v1_vn1,
           const obj::index_2_tuple_type& v2_vn2) {
            addPointNormalFace(
                {
                    std::get<0>(v0_vn0) - 1,
                    std::get<0>(v1_vn1) - 1,
                    std::get<0>(v2_vn2) - 1
                },
                {
                    std::get<1>(v0_vn0) - 1,
                    std::get<1>(v1_vn1) - 1,
                    std::get<1>(v2_vn2) - 1
                });
        },
        // triangular_face_geometric_vertices_texture_vertices_vertex_normals...
        [this](const obj::index_3_tuple_type& v0_vt0_vn0,
           const obj::index_3_tuple_type& v1_vt1_vn1,
           const obj::index_3_tuple_type& v2_vt2_vn2) {
            addPointNormalUvFace(
                {
                    std::get<0>(v0_vt0_vn0) - 1,
                    std::get<0>(v1_vt1_vn1) - 1,
                    std::get<0>(v2_vt2_vn2) - 1
                },
                {
                    std::get<1>(v0_vt0_vn0) - 1,
                    std::get<1>(v1_vt1_vn1) - 1,
                    std::get<1>(v2_vt2_vn2) - 1
                },
                {
                    std::get<2>(v0_vt0_vn0) - 1,
                    std::get<2>(v1_vt1_vn1) - 1,
                    std::get<2>(v2_vt2_vn2) - 1
                });
        },
        // quadrilateral_face_geometric_vertices_callback_type
        [](obj::index_type, obj::index_type, obj::index_type, obj::index_type) {
        },
        // quadrilateral_face_geometric_vertices_texture_vertices_callback_type
        [](const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&) {},
        // quadrilateral_face_geometric_vertices_vertex_normals_callback_type
        [](const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&) {},
        // quadrilateral_face_geometric_vertices_texture_vertices_vertex_norm...
        [](const obj::index_3_tuple_type&,
           const obj::index_3_tuple_type&,
           const obj::index_3_tuple_type&,
           const obj::index_3_tuple_type&) {},
        // polygonal_face_geometric_vertices_begin_callback_type
        [](obj::index_type, obj::index_type, obj::index_type) {},
        // polygonal_face_geometric_vertices_vertex_callback_type
        [](obj::index_type) {},
        // polygonal_face_geometric_vertices_end_callback_type
        []() {},
        // polygonal_face_geometric_vertices_texture_vertices_begin_callback_...
        [](const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&) {},
        // polygonal_face_geometric_vertices_texture_vertices_vertex_callback...
        [](const obj::index_2_tuple_type&) {},
        // polygonal_face_geometric_vertices_texture_vertices_end_callback_type
        []() {},
        // polygonal_face_geometric_vertices_vertex_normals_begin_callback_type
        [](const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&,
           const obj::index_2_tuple_type&) {},
        // polygonal_face_geometric_vertices_vertex_normals_vertex_callback_type
        [](const obj::index_2_tuple_type&) {},
        // polygonal_face_geometric_vertices_vertex_normals_end_callback_type
        []() {},
        // polygonal_face_geometric_vertices_texture_vertices_vertex_normals_...
        [](const obj::index_3_tuple_type&,
           const obj::index_3_tuple_type&,
           const obj::index_3_tuple_type&) {},
        // polygonal_face_geometric_vertices_texture_vertices_vertex_normals_...
        [](const obj::index_3_tuple_type&) {},
        // polygonal_face_geometric_vertices_texture_vertices_vertex_normals_...
        []() {});
    parser.group_name_callback([](const std::string&) {});
    parser.smoothing_group_callback([](obj::size_type) {});
    parser.object_name_callback([](const std::string&) {});
    parser.material_library_callback([](const std::string&) {});
    parser.material_name_callback([](const std::string&) {});
    parser.comment_callback([](const std::string&) {});

    return parser.parse(*strm);
}

TriangleMesh3& TriangleMesh3::operator=(const TriangleMesh3& other) {
    set(other);
    return *this;
}

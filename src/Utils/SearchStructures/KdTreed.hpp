/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020-2022, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KDTREE2D_H_
#define KDTREE2D_H_

#include <algorithm>
#include <vector>
#include <optional>
#include <cmath>
#include <glm/glm.hpp>

namespace sgl {

/**
 * An axis aligned (bounding) box data structures used for search queries.
 */
template<class T, int k>
class AxisAlignedBoxd {
public:
    using vec = glm::vec<k, T>;
    AxisAlignedBoxd() = default;
    AxisAlignedBoxd(const vec& min, const vec& max) : min(min), max(max) {}

    /// The minimum and maximum coordinate corners of the 3D cuboid.
    vec min{}, max{};

    /**
     * Tests whether the axis aligned box contains a point.
     * @param pt The point.
     * @return True if the box contains the point.
     */
    [[nodiscard]] inline bool contains(const vec& pt) const {
        for (int i = 0; i < k; i++) {
            if (pt[i] < min[i] || pt[i] > max[i]) {
                return false;
            }
        }
        return true;
    }
};
template<class T>
class AxisAlignedBoxd<T, 1> {
public:
    using vec = T;
    AxisAlignedBoxd() = default;
    AxisAlignedBoxd(vec min, vec max) : min(min), max(max) {}

    /// The minimum and maximum coordinate corners of the 3D cuboid.
    vec min{}, max{};

    /**
     * Tests whether the axis aligned box contains a point.
     * @param pt The point.
     * @return True if the box contains the point.
     */
    [[nodiscard]] inline bool contains(const vec& pt) const {
        return pt >= min && pt <= max;
    }
};

/**
 * A node in the k-d-tree. It stores in which axis the space is partitioned (x,y,z)
 * as an index, the position of the node, and its left and right children.
 */
template<class T, int k>
class KdNoded {
public:
    using vec = glm::vec<k, T>;
    KdNoded() : axis(0), left(nullptr), right(nullptr) {}
    int axis;
    vec point{};
    KdNoded* left;
    KdNoded* right;
};

enum class DistanceMeasure {
    EUCLIDEAN, CHEBYSHEV
};
template<DistanceMeasure d, class T, int k>
typename std::enable_if<d == DistanceMeasure::EUCLIDEAN, T>::type distanceMetric(const glm::vec<k, T>& diff) {
    T squaredSum = 0;
    for (int i = 0; i < k; i++) {
        squaredSum += diff[i] * diff[i];
    }
    return std::sqrt(squaredSum);
}
template<DistanceMeasure d, class T, int k>
typename std::enable_if<d == DistanceMeasure::CHEBYSHEV, T>::type distanceMetric(const glm::vec<k, T>& diff) {
    T largestDiff = std::abs(diff[0]);
    for (int i = 1; i < k; i++) {
        largestDiff = std::max(largestDiff, std::abs(diff[i]));
    }
    return largestDiff;
}
template<DistanceMeasure d, class T, int k = 1>
T distanceMetric(T diff) {
    return std::abs(diff);
}

/**
 * The k-d-tree class. Used for searching point sets in space efficiently.
 */
template<class T, int k, DistanceMeasure d>
class KdTreed
{
public:
    using vec = glm::vec<k, T>;
    KdTreed() : root(nullptr) {}
    ~KdTreed() = default;

    // Forbid the use of copy operations.
    KdTreed& operator=(const KdTreed& other) = delete;
    KdTreed(const KdTreed& other) = delete;

    /**
     * Clears the content of the k-d-tree.
     */
    void clear() {
        root = nullptr;
        nodes.clear();
        nodeCounter = 0;
    }

    /**
     * Builds a k-d-tree from the passed point array.
     * @param points The point array.
     * @param dataArray The data array.
     */
    void build(const std::vector<vec>& points) {
#ifdef TRACY_PROFILE_TRACING
        ZoneScoped;
#endif

        if (points.empty()) {
            return;
        }

        std::vector<vec> pointsCopy = points;
        nodes.resize(points.size());
        nodeCounter = 0;
        root = _build(pointsCopy, 0, 0, points.size());
    }

    /**
     * Builds a k-d-tree from the passed point array.
     * This version of the function uses the passed points array.
     * @param points The point array.
     * @param dataArray The data array.
     */
    void buildInplace(std::vector<vec>& points) {
#ifdef TRACY_PROFILE_TRACING
        ZoneScoped;
#endif

        if (points.empty()) {
            return;
        }

        nodes.resize(points.size());
        nodeCounter = 0;
        root = _build(points, 0, 0, points.size());
    }

    /**
     * Performs an area search in the k-d-tree and returns all points within a certain bounding box.
     * @param box The bounding box.
     * @param points The points stored in the k-d-tree inside of the bounding box.
     */
    void findPointsInAxisAlignedBox(const AxisAlignedBoxd<T, k>& box, std::vector<vec>& points) {
        _findPointsInAxisAlignedBox(box, points, root);
    }

    /**
     * Performs an area search in the k-d-tree and returns all points within a certain distance to some center point.
     * @param centerPoint The center point.
     * @param radius The search radius.
     * @return The points stored in the k-d-tree inside of the search radius.
     */
    void findPointsInSphere(const vec& center, T radius, std::vector<vec>& pointsWithDistance) {
        std::vector<vec> pointsInRect;

        // First, find all points within the bounding box containing the search circle.
        AxisAlignedBoxd<T, k> box;
        box.min = center - vec(radius, radius);
        box.max = center + vec(radius, radius);
        _findPointsInAxisAlignedBox(box, pointsInRect, root);

        // Now, filter all points out that are not within the search radius.
        vec differenceVector;
        for (const vec& point : pointsInRect) {
            if (distanceMetric<d, T, k>(point - center) <= radius) {
                pointsWithDistance.push_back(point);
            }
        }
    }

    /**
     * @param center The center point.
     * @param radius The search radius.
     * @return Whether there is at least one point stored in the k-d-tree inside of the search radius.
     */
    bool getHasPointCloserThan(const vec& center, T radius) {
#ifdef TRACY_PROFILE_TRACING
        ZoneScoped;
#endif

        vec nearestNeighbor;
        T nearestNeighborDistance = std::numeric_limits<T>::max();
        _findNearestNeighbor(center, nearestNeighborDistance, nearestNeighbor, root);
        if (nearestNeighborDistance <= radius) {
            return true;
        }
        return false;
    }

    /**
     * Performs an area search in the k-d-tree and returns the number of points within a certain bounding box.
     * @param box The bounding box.
     */
    size_t getNumPointsInAxisAlignedBox(const AxisAlignedBoxd<T, k>& box) {
        return _getNumPointsInAxisAlignedBox(box, root);
    }

    /**
     * Performs an area search in the k-d-tree and returns the number of points within a k-dimensional sphere.
     * @param center The center point.
     * @param radius The search radius.
     * @return The number of points stored in the k-d-tree inside of the search radius.
     */
    size_t getNumPointsInSphere(const vec& center, T radius) {
        return _getNumPointsInSphere(center, radius, root);
    }

    /**
     * Returns the nearest neighbor in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @return The closest neighbor within the maximum distance.
     */
    std::optional<vec> findNearestNeighbor(const vec& point) {
        T nearestNeighborDistance = std::numeric_limits<T>::max();
        vec nearestNeighbor;
        _findNearestNeighbor(point, nearestNeighborDistance, nearestNeighbor, root);
        return point;
    }

    /**
     * Returns the k nearest neighbors in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @param neighbors The k closest neighbors.
     * @param distances The distances to the k closest neighbors.
     */
    void findKNearestNeighbors(
            const vec& point, int kn, std::vector<vec>& neighbors, std::vector<T>& distances) {
        neighbors.resize(kn);
        distances.resize(kn, std::numeric_limits<T>::max());
        _findKNearestNeighbors(point, kn, neighbors, distances, root);
    }

    /**
     * Returns the k nearest neighbors in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @param distances The distances to the k closest neighbors.
     */
    void findKNearestNeighbors(const vec& point, int kn, std::vector<T>& distances) {
        distances.resize(kn, std::numeric_limits<T>::max());
        _findKNearestNeighbors(point, kn, distances, root);
    }

private:
    /// Root of the tree
    KdNoded<T, k>* root;

    // List of all nodes
    std::vector<KdNoded<T, k>> nodes;
    int nodeCounter = 0;

    /**
     * Builds a k-d-tree from the passed point array recursively (for internal use only).
     * @param points The point array.
     * @param dataArray The data array.
     * @param depth The current depth in the tree (starting at 0).
     * @return The parent node of the current sub-tree.
     */
    KdNoded<T, k>* _build(std::vector<vec>& points, int depth, size_t startIdx, size_t endIdx) {
        if (endIdx - startIdx == 0) {
            return nullptr;
        }

        KdNoded<T, k> *node = nodes.data() + nodeCounter;
        nodeCounter++;

        int axis = depth % k;
        std::sort(
                points.begin() + startIdx, points.begin() + endIdx,
                [axis](const vec& a, const vec& b) {
                    return a[axis] < b[axis];
                });
        size_t medianIndex = startIdx + (endIdx - startIdx) / 2;

        node->axis = axis;
        node->point = points.at(medianIndex);
        node->left = _build(points, depth + 1, startIdx, medianIndex);
        node->right = _build(points, depth + 1, medianIndex + 1, endIdx);
        return node;
    }

    /**
     * Performs an area search in the k-d-tree and returns the number of points within a certain bounding box
     * (for internal use only).
     * @param box The bounding box.
     * @param node The current k-d-tree node that is searched.
     */
    size_t _getNumPointsInAxisAlignedBox(const AxisAlignedBoxd<T, k>& box, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return 0;
        }

        size_t counter = 0;
        if (box.contains(node->point)) {
            counter = 1;
        }

        if (box.min[node->axis] <= node->point[node->axis]) {
            counter += _getNumPointsInAxisAlignedBox(box, node->left);
        }
        if (box.max[node->axis] >= node->point[node->axis]) {
            counter += _getNumPointsInAxisAlignedBox(box, node->right);
        }

        return counter;
    }

    /**
     * Performs an area search in the k-d-tree and returns the number of points within a k-dimensional sphere
     * (for internal use only).
     * @param center The center point.
     * @param radius The search radius.
     * @param node The current k-d-tree node that is searched.
     */
    size_t _getNumPointsInSphere(const vec& center, T radius, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return 0;
        }

        size_t counter = 0;
        if (distanceMetric<d, T, k>(node->point - center) <= radius) {
            counter = 1;
        }

        if (center[node->axis] - radius <= node->point[node->axis]) {
            counter += _getNumPointsInSphere(center, radius, node->left);
        }
        if (center[node->axis] + radius >= node->point[node->axis]) {
            counter += _getNumPointsInSphere(center, radius, node->right);
        }

        return counter;
    }

    /**
     * Performs an area search in the k-d-tree and returns all points within a certain bounding box
     * (for internal use only).
     * @param box The bounding box.
     * @param points The points of the k-d-tree inside of the bounding box.
     * @param node The current k-d-tree node that is searched.
     */
    void _findPointsInAxisAlignedBox(
            const AxisAlignedBoxd<T, k>& box, std::vector<vec>& points, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return;
        }

        if (box.contains(node->point)) {
            points.push_back(node->point);
        }

        if (box.min[node->axis] <= node->point[node->axis]) {
            _findPointsInAxisAlignedBox(box, points, node->left);
        }
        if (box.max[node->axis] >= node->point[node->axis]) {
            _findPointsInAxisAlignedBox(box, points, node->right);
        }
    }

    /**
     * Performs an area search in the k-d-tree and returns all points within a certain distance to some center point
     * (for internal use only).
     * @param centerPoint The center point.
     * @param radius The search radius.
     * @param points The points of the k-d-tree inside of the search radius.
     * @param node The current k-d-tree node that is searched.
     */
    void _findPointsInSphere(const vec& center, T radius, std::vector<vec>& points, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return;
        }

        if (distanceMetric<d, T, k>(node->point - center) <= radius) {
            points.push_back(node->point);
        }

        if (center[node->axis] - radius <= node->point[node->axis]) {
            _findPointsInSphere(center, radius, points, node->left);
        }
        if (center[node->axis] + radius >= node->point[node->axis]) {
            _findPointsInSphere(center, radius, points, node->right);
        }
    }

    /**
     * Returns the nearest neighbor in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @param nearestNeighborDistance The distance to the nearest neighbor found so far.
     * @param nearestNeighbor The nearest neighbor found so far.
     * @param node The current k-d-tree node that is searched.
     */
    void _findNearestNeighbor(
            const vec& point, T& nearestNeighborDistance,
            vec& nearestNeighbor, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return;
        }

        // Descend on side of split planes where the point lies.
        bool isPointOnLeftSide = point[node->axis] <= node->point[node->axis];
        if (isPointOnLeftSide) {
            _findNearestNeighbor(point, nearestNeighborDistance, nearestNeighbor, node->left);
        } else {
            _findNearestNeighbor(point, nearestNeighborDistance, nearestNeighbor, node->right);
        }

        // Compute the distance of this node to the point.
        T newDistance = distanceMetric<d, T, k>(point - node->point);
        if (newDistance < nearestNeighborDistance) {
            nearestNeighborDistance = newDistance;
            nearestNeighbor = node->point;
        }

        // Check whether there could be a closer point on the opposite side.
        if (isPointOnLeftSide && point[node->axis] + nearestNeighborDistance >= node->point[node->axis]) {
            _findNearestNeighbor(point, nearestNeighborDistance, nearestNeighbor, node->right);
        }
        if (!isPointOnLeftSide && point[node->axis] - nearestNeighborDistance <= node->point[node->axis]) {
            _findNearestNeighbor(point, nearestNeighborDistance, nearestNeighbor, node->left);
        }
    }

    /**
     * Returns the nearest neighbor in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @param neighbors The k closest neighbors.
     * @param distances The distances to the k closest neighbors.
     * @param node The current k-d-tree node that is searched.
     */
    void _findKNearestNeighbors(
            const vec& point, int kn, std::vector<vec>& neighbors, std::vector<T>& distances, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return;
        }

        // Descend on side of split planes where the point lies.
        bool isPointOnLeftSide = point[node->axis] <= node->point[node->axis];
        if (isPointOnLeftSide) {
            _findKNearestNeighbors(point, kn, neighbors, distances, node->left);
        } else {
            _findKNearestNeighbors(point, kn, neighbors, distances, node->right);
        }

        // Compute the distance of this node to the point.
        T newDistance = distanceMetric<d, T, k>(point - node->point);
        if (newDistance < distances.back()) {
            vec newVec = node->point;
            vec tempVec;
            T tempDistance;
            for (int i = 0; i < kn; i++) {
                if (newDistance < distances.at(i)) {
                    tempDistance = newDistance;
                    newDistance = distances.at(i);
                    distances.at(i) = tempDistance;
                    tempVec = newVec;
                    newVec = neighbors.at(i);
                    neighbors.at(i) = tempVec;
                }
            }
        }

        // Check whether there could be a closer point on the opposite side.
        if (isPointOnLeftSide && point[node->axis] + distances.back() >= node->point[node->axis]) {
            _findKNearestNeighbors(point, kn, neighbors, distances, node->right);
        }
        if (!isPointOnLeftSide && point[node->axis] - distances.back() <= node->point[node->axis]) {
            _findKNearestNeighbors(point, kn, neighbors, distances, node->left);
        }
    }

    /**
     * Returns the nearest neighbor in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @param distances The distances to the k closest neighbors.
     * @param node The current k-d-tree node that is searched.
     */
    void _findKNearestNeighbors(const vec& point, int kn, std::vector<T>& distances, KdNoded<T, k>* node) {
        if (node == nullptr) {
            return;
        }

        // Descend on side of split planes where the point lies.
        bool isPointOnLeftSide = point[node->axis] <= node->point[node->axis];
        if (isPointOnLeftSide) {
            _findKNearestNeighbors(point, kn, distances, node->left);
        } else {
            _findKNearestNeighbors(point, kn, distances, node->right);
        }

        // Compute the distance of this node to the point.
        T newDistance = distanceMetric<d, T, k>(point - node->point);
        if (newDistance < distances.back()) {
            T tempDistance;
            for (int i = 0; i < kn; i++) {
                if (newDistance < distances.at(i)) {
                    tempDistance = newDistance;
                    newDistance = distances.at(i);
                    distances.at(i) = tempDistance;
                }
            }
        }

        // Check whether there could be a closer point on the opposite side.
        if (isPointOnLeftSide && point[node->axis] + distances.back() >= node->point[node->axis]) {
            _findKNearestNeighbors(point, kn, distances, node->right);
        }
        if (!isPointOnLeftSide && point[node->axis] - distances.back() <= node->point[node->axis]) {
            _findKNearestNeighbors(point, kn, distances, node->left);
        }
    }
};

}

#endif //KDTREE2D_H_

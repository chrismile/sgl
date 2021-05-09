/*!
 * Renderable.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_GRAPHICS_SCENE_RENDERABLE_HPP_
#define SRC_GRAPHICS_SCENE_RENDERABLE_HPP_

#include <memory>

namespace sgl {

class DLL_OBJECT Renderable {
public:
    virtual ~Renderable() {}
    virtual void render() {}
};

typedef std::shared_ptr<Renderable> RenderablePtr;

}

/*! SRC_GRAPHICS_SCENE_RENDERABLE_HPP_ */
#endif

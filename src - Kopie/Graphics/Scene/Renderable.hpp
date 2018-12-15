/*!
 * Renderable.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_GRAPHICS_SCENE_RENDERABLE_HPP_
#define SRC_GRAPHICS_SCENE_RENDERABLE_HPP_

#include <boost/shared_ptr.hpp>

namespace sgl {

class Renderable {
public:
	virtual ~Renderable() {}
	virtual void render();
};

typedef boost::shared_ptr<Renderable> RenderablePtr;

}

/*! SRC_GRAPHICS_SCENE_RENDERABLE_HPP_ */
#endif

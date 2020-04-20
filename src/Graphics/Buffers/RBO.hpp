/*!
 * RBO.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_BUFFERS_RBO_HPP_
#define GRAPHICS_BUFFERS_RBO_HPP_

#include <Defs.hpp>
#include <boost/shared_ptr.hpp>

namespace sgl {

enum RenderbufferType {
    RBO_DEPTH16 = 0x81A5, RBO_DEPTH24_STENCIL8 = 0x88F0, RBO_DEPTH32F_STENCIL8 = 0x8CAD,
    RBO_RGBA8 = 0x8058
};

/*! RBOs can be attached to framebuffer objects (see FBO.hpp).
 *  For more information on RBOs see https://www.khronos.org/opengl/wiki/Renderbuffer_Object */

class DLL_OBJECT RenderbufferObject
{
public:
    RenderbufferObject() {}
    virtual ~RenderbufferObject() {}
    virtual int getWidth()=0;
    virtual int getHeight()=0;
    virtual int getSamples()=0;
};

typedef boost::shared_ptr<RenderbufferObject> RenderbufferObjectPtr;

}

/*! GRAPHICS_BUFFERS_RBO_HPP_ */
#endif

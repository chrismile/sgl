/*!
 * RBO.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#ifndef GRAPHICS_OPENGL_RBO_HPP_
#define GRAPHICS_OPENGL_RBO_HPP_

#include <Graphics/Buffers/RBO.hpp>

namespace sgl {

class DLL_OBJECT RenderbufferObjectGL : public RenderbufferObject
{
public:
	RenderbufferObjectGL(int _width, int _height, RenderbufferType rboType, int _samples = 0);
	virtual ~RenderbufferObjectGL();
	virtual int getWidth() { return width; }
	virtual int getHeight() { return height; }
	virtual int getSamples() { return samples; }
	inline GLuint getID() { return rbo; }

private:
	GLuint rbo;
	int width;
	int height;
	int samples;
};

}

/*! GRAPHICS_OPENGL_RBO_HPP_ */
#endif

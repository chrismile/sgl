/*!
 * Stream.hpp
 *
 *  Created on: 03.08.2015
 *      Author: Christoph Neuhauser
 */

#ifndef STREAM_HPP_
#define STREAM_HPP_

#include <cstdint>
#include <memory>

//! Standard size: 256 bytes
const size_t STD_BUFFER_SIZE = 256;

#include "BinaryStream.hpp"

namespace sgl {

typedef BinaryWriteStream WriteStream;
typedef BinaryReadStream ReadStream;
typedef std::shared_ptr<WriteStream> WriteStreamPtr;
typedef std::shared_ptr<ReadStream> ReadStreamPtr;

}

/*! STREAM_HPP_ */
#endif

/*
 * Stream.hpp
 *
 *  Created on: 03.08.2015
 *      Author: Christoph
 */

#ifndef STREAM_HPP_
#define STREAM_HPP_

#include <cstdint>
#include <boost/shared_ptr.hpp>

const size_t STD_BUFFER_SIZE = 256; // Standard size: 256 bytes

#include "BinaryStream.hpp"

namespace sgl {

typedef BinaryWriteStream WriteStream;
typedef BinaryReadStream ReadStream;
typedef boost::shared_ptr<WriteStream> WriteStreamPtr;
typedef boost::shared_ptr<ReadStream> ReadStreamPtr;

}

#endif /* STREAM_HPP_ */

#pragma once

#include <string>

#include "math/vec3.h"
#include "io/imagedatatype.h"
#include "stringutils.h"
#include "image.h"
#include "io/raw.h"

using std::string;
using math::Vec3c;
using itl2::toString;
using itl2::imageDataType;

namespace pilib
{
	/**
	Base class for distributed images.
	*/
	class DistributedImageBase
	{
	private:

		/**
		Dimensions of the image.
		*/
		Vec3c dims;

		/**
		Name of the image in the system, i.e., name of variable holding this image.
		*/
		string name;

		/**
		Filename where the image data should be read.
		*/
		string readSource;

		/**
		Filename where modified image data should be saved.
		*/
		string writeTarget;

		/**
		Names of temporary files where image data is stored temporarily.
		These files are system-generated, so they will be deleted when the image is deleted.
		*/
		string tempFilename1, tempFilename2;

		/**
		Flag that indicates that this image is new (should not be read from temp file)
		*/
		bool isNewImage;

		/**
		Pixel data type
		*/
		string dataTypeStr;

		/**
		Generate filename for temporary storage.
		If source file is raw, the temp file is raw.
		Otherwise, the temp file is folder containing an image sequence.
		*/
		void createTempFilenames();

	protected:

		bool isSavedToDisk() const
		{
			return !isNewImage;
		}

	public:

		/**
		Creates distributed image that points to given file.
		*/
		DistributedImageBase(const string& name, coord_t width, coord_t height, coord_t depth, const string& dataTypeStr, const string& sourceFilename) :
			dims(width, height, depth),
			name(name),
			dataTypeStr(dataTypeStr)
		{
			setReadSource(sourceFilename);
			createTempFilenames();
		}

		/**
		Creates distributed image using temporary file as storage.
		*/
		DistributedImageBase(const string& name, coord_t width, coord_t height, coord_t depth, const string& dataType) :
			DistributedImageBase(name, width, height, depth, dataType, "")
		{

		}

		virtual ~DistributedImageBase();

		/**
		Gets dimensions of this image.
		*/
		Vec3c dimensions() const
		{
			return dims;
		}

		coord_t width() const
		{
			return dims.x;
		}

		coord_t height() const
		{
			return dims.y;
		}

		coord_t depth() const
		{
			return dims.z;
		}

		/**
		Gets count of pixels in the image.
		*/
		coord_t pixelCount() const
		{
			return dims.x * dims.y * dims.z;
		}

		/**
		Gets data type of this image.
		*/
		ImageDataType dataType() const
		{
			return fromString(dataTypeStr);
		}

		/**
		Gets piece of pi2 code to read a block of this image.
		@param dataNeeded Set to true if the image is used as input data. In that case the image data should be read from disk.
		*/
		string emitReadBlock(const Vec3c& filePos, const Vec3c& blockSize, bool dataNeeded) const;

		/**
		Gets piece of pi2 code to write a block of this image.
		@param outputFile Name of file to write. Set to null to indicate writing to internally determined (temporary) image.
		*/
		string emitWriteBlock(const Vec3c& filePos, const Vec3c& imagePos, const Vec3c& blockSize, const string* outputFile);

		/**
		Call when all blocks of this image have been written.
		*/
		void writeComplete()
		{
			setReadSource(writeTarget);
		}

		/**
		Gets the file path where the image data should be read.
		*/
		string currentReadSource() const
		{
			return readSource;
		}

		/**
		Gets the file path where the changed image will be saved.
		*/
		string currentWriteTarget() const
		{
			return writeTarget;
		}

		/**
		Enures that image writing process does not override old data (that might be needed by another processes if doing distribution with overlapping blocks).
		Swaps between two write targets.
		*/
		void newWriteTarget()
		{
			if (writeTarget == tempFilename1)
				writeTarget = tempFilename2;
			else
				writeTarget = tempFilename1;
		}

		/**
		Gets a value indicating whether the current file read location is a temporary file.
		*/
		bool savedToTemp() const
		{
			return currentReadSource() == tempFilename1 || currentReadSource() == tempFilename2;
		}

		/**
		Changes the location where the image is currently stored.
		*/
		void setReadSource(const string& filename);

		/**
		Gets name of variable that stores this image.
		*/
		const string& varName() const
		{
			return name;
		}

		void ensureSize(const Vec3c& newDimensions);
		
		void ensureSize(coord_t w, coord_t h = 1, coord_t d = 1)
		{
			ensureSize(Vec3c(w, h, d));
		}


		bool isRaw() const
		{
			return endsWith(currentReadSource(), ".raw");
		}

		bool isSequence() const
		{
			return !isRaw();
		}

	};

	template<typename pixel_t> class DistributedImage : public DistributedImageBase
	{
	public:
		/**
		Creates distributed image whose source points to the given file.
		*/
		DistributedImage(const string& name, coord_t width, coord_t height, coord_t depth, const string& filename) :
			DistributedImageBase(name, width, height, depth, toString(imageDataType<pixel_t>()), filename)
		{

		}

		/**
		Creates distributed image without source file.
		*/
		DistributedImage(const string& name, coord_t width = 1, coord_t height = 1, coord_t depth = 1) :
			DistributedImageBase(name, width, height, depth, toString(imageDataType<pixel_t>()))
		{

		}

		/**
		Reads the data of this distributed image to the given normal image.
		*/
		void readTo(itl2::Image<pixel_t>& img)
		{
			img.ensureSize(dimensions());

			if (isSavedToDisk())
			{
				if (endsWith(currentReadSource(), ".raw"))
				{
					raw::read(img, currentReadSource());
				}
				else
				{
					sequence::read(img, currentReadSource());
				}
			}
		}

		/**
		Copies data of given normal image to this distributed image.
		*/
		void setData(const itl2::Image<pixel_t>& img)
		{
			ensureSize(img.dimensions());

			if (endsWith(currentWriteTarget(), ".raw"))
			{
				raw::write(img, currentWriteTarget());
			}
			else
			{
				sequence::write(img, currentWriteTarget());
			}

			writeComplete();
		}
	};
}

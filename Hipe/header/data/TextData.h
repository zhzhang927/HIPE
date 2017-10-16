#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <data/IODataType.h>
#include <data/IOData.h>
#include <data/ImageData.h>


namespace data
{
	class TextData : public IOData<Data, TextData>
	{
	protected:
		TextData()
		{

		}

	public:
		using IOData::IOData;

	public:
		TextData(ImageData picture) : IOData(IODataType::TXT)
		{
			Data::registerInstance(new TextData());


			This()._type = TXT;
		}

		TextData(ImageData picture, std::vector<int> squareCrop) : IOData(IODataType::SQR_CROP)
		{
			Data::registerInstance(new TextData());

			This()._type = TXT;

		}

		TextData(const TextData& left) : IOData(left.getType())
		{
			Data::registerInstance(left._This);

		}


	};
}
